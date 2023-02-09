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

#define TEMPERATURE_SENSOR_PIN 5
#define MAX_MESSAGE_LENGTH 50
#define AVG_NUM_TEMP 10

SemaphoreHandle_t conexaoWifiSemaphore;
SemaphoreHandle_t conexaoMQTTSemaphore;
SemaphoreHandle_t envioMqttMutex;

QueueHandle_t fila_temperatura;

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
    xTaskCreate(&trataSensorDeTemperatura, "Leitura Sensor Temperatura", 2096, NULL, 1, NULL);
    xTaskCreate(&trataMediaTemperaturaHumidade, "Calculo Média Temperatura Envio MQTT", 4096, NULL, 1, NULL);

    while(true)
    {
      sprintf(JsonAtributos, "{\"quantidade de pinos\": 5}");

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
    wifi_start();

    fila_temperatura = xQueueCreate(AVG_NUM_TEMP, sizeof(TemperatureData));

    xTaskCreate(&conectadoWifi,  "Conexão ao MQTT", 4096, NULL, 1, NULL);
    xTaskCreate(&trataComunicacaoComServidor, "Comunicação com Broker", 4096, NULL, 1, NULL);
}
