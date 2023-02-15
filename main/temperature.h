#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#define TEMPERATURE_SENSOR_PIN CONFIG_TEMPERATURE_SENSOR_PIN

typedef struct TemperatureData {
  int temperature;
  int humidity;
} TemperatureData;

bool has_temperature_sensor();

void setup_temperature();

void handle_temperature_sensor(void * params);

void handle_average_temperature(void * params);

#endif
