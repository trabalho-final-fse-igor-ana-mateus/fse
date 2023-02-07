#include <string.h>

#include "handler.h"
#include "cjson.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define GPIO_PIN02 2
#define GPIO_PIN04 4

void handleJSON(const char *resp){
  cJSON *jsonResp = cJSON_Parse(resp);
  cJSON *value = cJSON_GetObjectItemCaseSensitive(jsonResp, "params");


  //verifica parâmetro da luz
  cJSON *luz = cJSON_GetObjectItemCaseSensitive(value, "luz");
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

