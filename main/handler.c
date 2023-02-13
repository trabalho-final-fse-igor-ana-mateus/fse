#include <string.h>

#include "handler.h"
#include "cjson.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define GPIO_PIN0 0
#define GPIO_PIN02 2
#define GPIO_PIN04 4
#define GPIO_PIN5 5
#define GPIO_PIN16 16
#define GPIO_PIN18 18

int luzEstado = 0;
int estado_sensor_toque = 0;
int estado_sensor_inclinacao = 0;

SemaphoreHandle_t semaphoreLuz;

QueueHandle_t filaDeInterrupcao;

static void IRAM_ATTR gpio_isr_handler(void *args)
{
  int pino = (int)args;
  xQueueSendFromISR(filaDeInterrupcao, &pino, NULL);
}


void setup(){
  esp_rom_gpio_pad_select_gpio(GPIO_PIN0);
  esp_rom_gpio_pad_select_gpio(GPIO_PIN02);
  esp_rom_gpio_pad_select_gpio(GPIO_PIN04);
  esp_rom_gpio_pad_select_gpio(GPIO_PIN5);
  esp_rom_gpio_pad_select_gpio(GPIO_PIN16);
  esp_rom_gpio_pad_select_gpio(GPIO_PIN18);

  gpio_set_direction(GPIO_PIN02, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_PIN04, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_PIN16, GPIO_MODE_OUTPUT);

  gpio_set_direction(GPIO_PIN0, GPIO_MODE_INPUT);
  gpio_set_direction(GPIO_PIN5, GPIO_MODE_INPUT);
  gpio_set_direction(GPIO_PIN18, GPIO_MODE_INPUT);

  gpio_set_pull_mode(GPIO_PIN5, GPIO_PULLDOWN_ONLY);

  gpio_set_intr_type(GPIO_PIN5, GPIO_INTR_POSEDGE);
  
  semaphoreLuz = xSemaphoreCreateBinary();

  filaDeInterrupcao = xQueueCreate(10, sizeof(int));

  xTaskCreate(&piscaLed,  "Led bicolor", 2048, NULL, 1, NULL);
  xTaskCreate(&detectaToque,  "Detecta toque", 2048, NULL, 1, NULL);
  xTaskCreate(&trataSensorInclinacao,  "Trata botao", 2048, NULL, 1, NULL);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(GPIO_PIN5, gpio_isr_handler, (void *) GPIO_PIN5); 
}

void trataSensorInclinacao(void * params){
  while (true)
  {
    estado_sensor_inclinacao = gpio_get_level(GPIO_PIN18);
    // printf("estado_sensor_inclinacao: %d\n", estado_sensor_inclinacao);
    gpio_set_level(GPIO_PIN02, estado_sensor_inclinacao);
    vTaskDelay(100 / portTICK_PERIOD_MS);    
  } 
}

void piscaLed(void * params)
{
  while(true) 
  {
    // printf("Estado luzEstado = %d\n", luzEstado);
    // printf("Estado estado_sensor_toque = %d\n", estado_sensor_toque);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    if(luzEstado == 1 && estado_sensor_toque == 0 && estado_sensor_inclinacao == 0){
      // printf("luzEstado: %d\n", luzEstado);
      gpio_set_level(GPIO_PIN16, 0);
      gpio_set_level(GPIO_PIN04, 1);
    }else if(luzEstado == 1 && (estado_sensor_toque == 1 || estado_sensor_inclinacao == 1)){
      gpio_set_level(GPIO_PIN04, 0);
      gpio_set_level(GPIO_PIN16, 1);
    }else{
      gpio_set_level(GPIO_PIN04, 0);
      gpio_set_level(GPIO_PIN16, 0);
    }
  }
}

void detectaToque(void * params){
  int pino;

  while(true){
    //printf("Entrou na task de toque, sinal %d\n", gpio_get_level(GPIO_PIN5));
    if(xQueueReceive(filaDeInterrupcao, &pino, portMAX_DELAY)){
      // De-bouncing
      int estado = gpio_get_level(pino);
      estado_sensor_toque = estado;
      if(estado == 1){
        gpio_isr_handler_remove(pino);
        while(gpio_get_level(pino) == estado){
          vTaskDelay(50 / portTICK_PERIOD_MS);
        }
       
        estado_sensor_toque = 0;
        // Habilitar novamente a interrupção
        vTaskDelay(50 / portTICK_PERIOD_MS);
        gpio_isr_handler_add(pino, gpio_isr_handler, (void *) pino);
      }
    }
  }
}

void handleJSON(const char *resp){
  cJSON *jsonResp = cJSON_Parse(resp);
  cJSON *value = cJSON_GetObjectItemCaseSensitive(jsonResp, "params");

  //verifica parâmetro da luz
  cJSON *luz = cJSON_GetObjectItemCaseSensitive(value, "luz");
  // printf("Estado da luz: %d\n", luz->valueint);
  luzEstado = luz->valueint;
  // printf("luzEstado: %d\n", luzEstado);
}

