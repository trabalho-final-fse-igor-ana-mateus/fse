#ifndef SOUND_DETECTOR_H
#define SOUND_DETECTOR_H

#define SOUND_DETECTOR_ANALOG_PIN CONFIG_SOUND_DETECTOR_PIN

bool has_sound_detector_sensor();

void sound_detector_setup();

void sound_detector_verify_task(void * params);

#endif
