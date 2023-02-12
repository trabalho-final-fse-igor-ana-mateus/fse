#ifndef FLAME_DETECTOR_H
#define FLAME_DETECTOR_H

void flame_detector_setup();

void flame_detector_posedge_handler();

bool get_flame_alarm_on();

void set_flame_alarm_on_to(bool value);

void flame_detector_alarm_button_handler();

#endif
