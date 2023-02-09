#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/semphr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "temperature.h"
#include "mqtt.h"
#include "dht11.h"

#define TEMPERATURE_SENSOR_PIN 5
#define MAX_MESSAGE_LENGTH 50
#define AVG_NUM_TEMP 10

extern SemaphoreHandle_t envioMqttMutex;

extern QueueHandle_t fila_temperatura;

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