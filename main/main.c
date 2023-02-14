#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "freertos/semphr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi.h"
#include "mqtt.h"
#include "dht11.h"
#include "temperature.h"
#include "flame_detector.h"

#define INTERRUPTION_QUEUE_SIZE 15

SemaphoreHandle_t conexaoWifiSemaphore;
SemaphoreHandle_t conexaoMQTTSemaphore;
SemaphoreHandle_t envioMqttMutex;

QueueHandle_t interruption_queue;

extern QueueHandle_t fila_temperatura;

static void IRAM_ATTR gpio_isr_handler(void * args) {
  int pin = (int) args;
  
  xQueueSendFromISR(interruption_queue, &pin, NULL);
}

void handle_interruption(void * params) {
    int pin;

    while (true) {
      if (xQueueReceive(interruption_queue, &pin, portMAX_DELAY)) {
        printf("Interrupt in pin %d\n", pin);

        if (pin == FLAME_DETECTOR_DIGITAL_PIN) {
          if (gpio_get_level(FLAME_DETECTOR_DIGITAL_PIN)) {
            flame_detector_posedge_handler();
          }
        }
      }
    }
}

void conectadoWifi(void * params)
{
  while(true) 
  {
    if(xSemaphoreTake(conexaoWifiSemaphore, portMAX_DELAY))
    {
      ESP_LOGI("[Network]", "Conexao wi-fi estabelecida");
      // Processamento Internet
      mqtt_start();
      ESP_LOGI("[Mqtt_broker]", "Comunicação com o broker: mqtt://164.41.98.25 iniciada");
    }
  }
}

void trataComunicacaoComServidor(void * params)
{
  char JsonAtributos[200];
  
  if(xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
  {
    if (has_temperature_sensor()) {
      xTaskCreate(&handle_temperature_sensor, "Leitura Sensor Temperatura", 2048, NULL, 1, NULL);
      xTaskCreate(&handle_average_temperature, "Calculo Média Temperatura Envio MQTT", 4096, NULL, 1, NULL);
    }

    while(true)
    {
      sprintf(JsonAtributos, "{\"quantidade de pinos\": 5, \"choque\": false}");

      if(xSemaphoreTake(envioMqttMutex, portMAX_DELAY)) {
        mqtt_envia_mensagem("v1/devices/me/attributes", JsonAtributos);

        xSemaphoreGive(envioMqttMutex);
      }
      
      vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
  }
}

void app_main(void)
{
    // Inicializa o NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    conexaoWifiSemaphore = xSemaphoreCreateBinary();
    conexaoMQTTSemaphore = xSemaphoreCreateBinary();
    envioMqttMutex = xSemaphoreCreateMutex();
    
    setup_temperature();

    flame_detector_setup();

    wifi_start();

    interruption_queue = xQueueCreate(INTERRUPTION_QUEUE_SIZE, sizeof(int));
    xTaskCreate(&handle_interruption,  "Trata interrupções", 2048, NULL, 1, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(FLAME_DETECTOR_DIGITAL_PIN, gpio_isr_handler, (void *) FLAME_DETECTOR_DIGITAL_PIN);

    xTaskCreate(&conectadoWifi,  "Conexão ao MQTT", 4096, NULL, 1, NULL);
    xTaskCreate(&trataComunicacaoComServidor, "Comunicação com Broker", 4096, NULL, 1, NULL);
}
