#ifndef FLAME_DETECTOR_H
#define FLAME_DETECTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/semphr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/adc.h"
#include "driver/ledc.h"
#include "pthread.h"
#include "esp_pthread.h"

#include "flame_detector.h"

#define FLAME_DETECTOR_ALARM_LED_PIN 2
#define FLAME_DETECTOR_DIGITAL_PIN 23
#define FLAME_DETECTOR_TURN_OFF_ALARM_BUTTON 0

bool flame_alarm_on = false;

SemaphoreHandle_t flame_alarm_mutex;

bool get_flame_alarm_on() {
    bool ret = false;

    if(xSemaphoreTake(flame_alarm_mutex, portMAX_DELAY)) {
        ret = flame_alarm_on;

        xSemaphoreGive(flame_alarm_mutex);
    }

    return ret;
}

void set_flame_alarm_on_to(bool value) {
    if(xSemaphoreTake(flame_alarm_mutex, portMAX_DELAY)) {
        flame_alarm_on = value;

        xSemaphoreGive(flame_alarm_mutex);
    }
}

void flame_detector_setup() {
    // BOTÃO ALARME
    
    esp_rom_gpio_pad_select_gpio(FLAME_DETECTOR_TURN_OFF_ALARM_BUTTON);
    gpio_set_direction(FLAME_DETECTOR_TURN_OFF_ALARM_BUTTON, GPIO_MODE_INPUT);

    // Habilita pulldown
    gpio_pulldown_en(FLAME_DETECTOR_TURN_OFF_ALARM_BUTTON);
    // Desabilita pullup por segurança
    gpio_pullup_dis(FLAME_DETECTOR_TURN_OFF_ALARM_BUTTON);

    // Configura detecção de borda de subida
    gpio_set_intr_type(FLAME_DETECTOR_TURN_OFF_ALARM_BUTTON, GPIO_INTR_ANYEDGE);

    // DETECTOR DE CHAMA

    esp_rom_gpio_pad_select_gpio(FLAME_DETECTOR_DIGITAL_PIN);
    gpio_set_direction(FLAME_DETECTOR_DIGITAL_PIN, GPIO_MODE_INPUT);

    // Habilita pulldown
    gpio_pulldown_en(FLAME_DETECTOR_DIGITAL_PIN);
    // Desabilita pullup por segurança
    gpio_pullup_dis(FLAME_DETECTOR_DIGITAL_PIN);

    // Configura detecção de borda de subida
    gpio_set_intr_type(FLAME_DETECTOR_DIGITAL_PIN, GPIO_INTR_POSEDGE);

    // Configura PWM do led de alarme
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ledc_timer_config(&timer_config);

    ledc_channel_config_t channel_config = {
        .gpio_num = FLAME_DETECTOR_ALARM_LED_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };

    ledc_channel_config(&channel_config);

    // Cria mutex para a região crítica da varíavel do alarme
    flame_alarm_mutex = xSemaphoreCreateMutex();
}

void turn_on_led_alarm_till_is_off(void * args) {
    ledc_fade_func_install(0);

    while (true) {
        if (!get_flame_alarm_on()) {
            gpio_set_level(FLAME_DETECTOR_ALARM_LED_PIN, 0);
            break;
        }

        ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, 1000, LEDC_FADE_WAIT_DONE);

        ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 255, 1000, LEDC_FADE_WAIT_DONE);
    }

    ledc_fade_func_uninstall();
    pthread_exit(NULL);
}

void flame_detector_turn_on_alarm() {
    if (!get_flame_alarm_on()) {
        pthread_t tid;
        set_flame_alarm_on_to(true);

        pthread_create(&tid, 0, &turn_on_led_alarm_till_is_off, NULL);

        pthread_detach(tid);
    }
}

void flame_detector_posedge_handler() {
    flame_detector_turn_on_alarm();
}

void flame_detector_alarm_button_handler() {
    if (get_flame_alarm_on()) {
        set_flame_alarm_on_to(false);
        ledc_fade_func_uninstall();
        gpio_set_level(FLAME_DETECTOR_ALARM_LED_PIN, 0);
    }
}

#endif