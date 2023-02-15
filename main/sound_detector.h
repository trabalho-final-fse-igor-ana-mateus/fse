#ifndef SOUND_DETECTOR_H
#define SOUND_DETECTOR_H

bool has_sound_detector_sensor();

void sound_detector_setup();

void turn_on_laser();

void turn_off_laser();

void sound_detector_verify_task(void * params);

void sound_detector_read_state_from_nvs();

#endif
