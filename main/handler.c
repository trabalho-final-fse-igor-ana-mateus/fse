#include <string.h>

#include "handler.h"
#include "cJSON.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "flame_detector.h"

#define GPIO_PIN02 2
#define GPIO_PIN04 4

void handleJSON(const char * response) {
  cJSON * json = cJSON_Parse(response);

  printf("response json:\n\t%s\n", cJSON_PrintUnformatted(json));

  cJSON * params = cJSON_GetObjectItemCaseSensitive(json, "params");
  char * action =
    cJSON_HasObjectItem(params, "action")
      ? cJSON_GetObjectItem(params, "action")->valuestring
      : "";

  if (strcmp(action, "turn_off_fire_alarm") == 0) {
    flame_detector_turn_off_alarm();
  } else {
    //verifica parâmetro da luz
    cJSON *luz = cJSON_GetObjectItemCaseSensitive(params, "luz");

    if (luz) {
      printf("Estado da luz: %s\n", luz->valuestring);

      gpio_set_direction(GPIO_PIN02, GPIO_MODE_OUTPUT);

      if(strcmp("true", luz->valuestring) == 0) {
        printf("Luz ligada\n");
        gpio_set_level(GPIO_PIN02, 1);
        
      }else{
        printf("Luz desligada\n");
        gpio_set_level(GPIO_PIN02, 0);

      }
    }
  }
}

