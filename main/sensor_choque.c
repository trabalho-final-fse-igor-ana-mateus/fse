#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define GPIO_SENSOR_INPUT 4
#define GPIO_LED_OUTPUT 2

void sensor_task(void *pvParameter) {
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_POSEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1<<GPIO_SENSOR_INPUT);
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 1;
  gpio_config(&io_conf);

  io_conf.intr_type = GPIO_INTR_POSEDGE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1<<GPIO_LED_OUTPUT);
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  int last_sensor_state = 0;
  while (true) {
    int sensor_state = gpio_get_level(GPIO_SENSOR_INPUT);
    if (sensor_state == 1 && last_sensor_state == 0) {
      // Detecção de borda de descida
      gpio_set_level(GPIO_LED_OUTPUT, 1);
    } else if (sensor_state == 0 && last_sensor_state == 1) {
      // Detecção de borda de subida
      gpio_set_level(GPIO_LED_OUTPUT, 0);
    }
    last_sensor_state = sensor_state;
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

// void app_main() {
//   xTaskCreate(sensor_task, "sensor_task", 2048, NULL, 10, NULL);
// }