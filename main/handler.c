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
#define GPIO_PIN5 5
#define GPIO_PIN16 16

char luzEstado[10];
SemaphoreHandle_t semaphoreLuz;

void setup(){
  gpio_set_direction(GPIO_PIN02, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_PIN04, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_PIN16, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_PIN5, GPIO_MODE_INPUT);
  

  memset(luzEstado, '\0', sizeof luzEstado);

  semaphoreLuz = xSemaphoreCreateBinary();

  xTaskCreate(&piscaLed,  "Led bicolor", 4096, NULL, 1, NULL);
  xTaskCreate(&detectaToque,  "Detecta toque", 4096, NULL, 1, NULL);
}

void piscaLed(void * params)
{
  while(true) 
  {
    if(strcmp("true", luzEstado) == 0){
      printf("luzEstado: %d\n", luzEstado);
      gpio_set_level(GPIO_PIN04, 1);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      gpio_set_level(GPIO_PIN04, 0);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      gpio_set_level(GPIO_PIN16, 1);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      gpio_set_level(GPIO_PIN16, 0);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }else{
      gpio_set_level(GPIO_PIN04, 0);
      gpio_set_level(GPIO_PIN16, 0);
    }
  }
}

void detectaToque(void * params){
  while(true){
    printf("Entrou na task de toque, sinal %d\n", gpio_get_level(GPIO_PIN5));
    if(gpio_get_level(GPIO_PIN5) == 1){
      printf("Entrou na task de toque, sinal 1");
    }else{
      gpio_set_level(GPIO_PIN16, 0);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void handleJSON(const char *resp){
  //setup();
  // cJSON *jsonResp = cJSON_Parse(resp);
  // cJSON *value = cJSON_GetObjectItemCaseSensitive(jsonResp, "params");

  // //verifica parâmetro da luz
  // cJSON *luz = cJSON_GetObjectItemCaseSensitive(value, "luz");
  // printf("Estado da luz: %s\n", luz->valuestring);

  // if(strcmp("true", luz->valuestring) == 0) {
  //   printf("Luz ligada\n");
  //   sprintf(luzEstado, "true");
  //   gpio_set_level(GPIO_PIN02, 1);
  // }else{
  //   printf("Luz desligada\n");
  //   gpio_set_level(GPIO_PIN02, 0);

  // }
}

