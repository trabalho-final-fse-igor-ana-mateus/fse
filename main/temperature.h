#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#define TEMPERATURE_SENSOR_PIN CONFIG_TEMPERATURE_SENSOR_PIN

typedef struct TemperatureData {
  int temperature;
  int humidity;
} TemperatureData;

bool has_temperature_sensor();

void setup_temperature();

void trataSensorDeTemperatura(void * params);

void trataMediaTemperaturaHumidade(void * params);

#endif
