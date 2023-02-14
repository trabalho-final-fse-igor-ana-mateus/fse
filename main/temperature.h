#ifndef TEMPERATURE_H
#define TEMPERATURE_H

typedef struct TemperatureData {
  int temperature;
  int humidity;
} TemperatureData;

void setup_temperature();

void trataSensorDeTemperatura(void * params);

void trataMediaTemperaturaHumidade(void * params);

#endif
