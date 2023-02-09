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

#define TEMPERATURE_SENSOR_PIN 5
#define MAX_MESSAGE_LENGTH 50
#define AVG_NUM_TEMP 10

SemaphoreHandle_t conexaoWifiSemaphore;
SemaphoreHandle_t conexaoMQTTSemaphore;
SemaphoreHandle_t envioMqttMutex;

QueueHandle_t fila_temperatura;

typedef struct TemperatureData {
  int temperature;
  int humidity;
} TemperatureData;

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

void trataSensorDeTemperatura(void * params) {
  DHT11_init(TEMPERATURE_SENSOR_PIN);
  
  while(true) {
    struct dht11_reading read_data = DHT11_read();
    TemperatureData temperature_data;

    if (read_data.status == DHT11_OK) {
      temperature_data.temperature = read_data.temperature;
      temperature_data.humidity = read_data.humidity;

      printf("TEMPERATURE: %d %d\n", temperature_data.temperature, temperature_data.humidity);

      long ret = xQueueSend(fila_temperatura, &temperature_data, 1000 / portTICK_PERIOD_MS);

      if (!ret) {
        ESP_LOGE("TEMPERATURE", "Falha ao adicionar temperatura na fila\n");
      }
    } else {
      ESP_LOGE("TEMPERATURE", "Falha na leitura de temperatura e humidade");
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void trataMediaTemperaturaHumidade(void * params) {
  int counter = 1;
  char mensagem[MAX_MESSAGE_LENGTH];
  TemperatureData current_data;
  TemperatureData prev_data = { .temperature = 0, .humidity = 0 };

  while (true) {
    if (xQueueReceive(fila_temperatura, &current_data, 1000 / portTICK_PERIOD_MS)) {
      float avg_temperature;
      float avg_humidity;

      avg_temperature = (prev_data.temperature + current_data.temperature) / 2.0;
      avg_humidity = (prev_data.humidity + current_data.humidity) / 2.0;

      printf("MEDIA_TEMPERATURE: %.2f %.2f\n", avg_temperature, avg_humidity);

      if (counter % 10 == 0) {
        memset(mensagem, 0x0, MAX_MESSAGE_LENGTH * sizeof(char));

        sprintf(
          mensagem,
          "{\"temperature\": %.2f,\"humidity\": %.2f}",
          avg_temperature,
          avg_humidity
        );

        printf("MEDIA_TEMPERATURE: %s\n", mensagem);
        
        if(xSemaphoreTake(envioMqttMutex, portMAX_DELAY)) {
          mqtt_envia_mensagem("v1/devices/me/telemetry", mensagem);

          xSemaphoreGive(envioMqttMutex);
        }

        counter = 1;
        xQueueReset(fila_temperatura);
      }

      prev_data.temperature = current_data.temperature;
      prev_data.humidity = current_data.humidity;
    } else {
      ESP_LOGE("MEDIA_TEMPERATURE", "Falha ao ler temperatura da fila\n");
    }

    counter++;

    vTaskDelay(1000 / portTICK_PERIOD_MS);
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
