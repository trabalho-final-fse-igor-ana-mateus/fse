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

#include "sound_detector.h"
#include "nvs_helper.h"
#include "mqtt.h"

#define SOUND_DETECTOR_VALUE_THRESHOLD 60
#define LASER_PIN CONFIG_LASER_PIN
#define SOUND_DETECTOR_ANALOG_PIN ADC1_CHANNEL_5

extern SemaphoreHandle_t envioMqttMutex;

bool has_sound_detector_sensor() {
    return SOUND_DETECTOR_ANALOG_PIN > 0;
}

void sound_detector_setup() {
    if (!has_sound_detector_sensor()) return;

    // CONFIGURAÇÃO DETECTOR DE SOM

    // aparentemente essa função não existe mais e o width é passado na get_raw 
    // adc1_config(ADC_WIDTH_MAX);
    // não sei o que esse ADC_ATTEN_DB_xx é exatamente
    adc1_config_width(ADC_WIDTH_BIT_10);
    adc1_config_channel_atten(SOUND_DETECTOR_ANALOG_PIN, ADC_ATTEN_DB_0);
    
    if (LASER_PIN > 0) {
        esp_rom_gpio_pad_select_gpio(LASER_PIN);
        gpio_set_direction(LASER_PIN, GPIO_MODE_OUTPUT);
    }
}

void turn_on_laser() {
    gpio_set_level(LASER_PIN, 1);
    nvs_write_int_value("laser", 1);

    if(xSemaphoreTake(envioMqttMutex, portMAX_DELAY)) {
        mqtt_envia_mensagem("v1/devices/me/attributes", "{\"laser\": true}");
        xSemaphoreGive(envioMqttMutex);
    }
}

void turn_off_laser() {
    gpio_set_level(LASER_PIN, 0);
    nvs_write_int_value("laser", 0);
    
    if(xSemaphoreTake(envioMqttMutex, portMAX_DELAY)) {
        mqtt_envia_mensagem("v1/devices/me/attributes", "{\"laser\": false}");
        xSemaphoreGive(envioMqttMutex);
    }
}

void sound_detector_verify_task(void * params) {
    int sound_value = 0;

    while (true) {
        sound_value = adc1_get_raw(SOUND_DETECTOR_ANALOG_PIN);

        printf("\nSOUND VALUE: %d\n", sound_value);

        if (sound_value >= SOUND_DETECTOR_VALUE_THRESHOLD) {
            printf("Som passou do limiar configurado: %d\n", sound_value);

            turn_on_laser();
            
            vTaskDelay(10000 / portTICK_PERIOD_MS);

            turn_off_laser();
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void sound_detector_read_state_from_nvs() {
    int value;

    if (nvs_read_int_value("laser", &value) && value == 1) {
        turn_on_laser();
    }

    turn_off_laser();
}
