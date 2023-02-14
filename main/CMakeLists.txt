set(COMPONENT_SRCS "nvs_helper.c" "sound_detector.c" "flame_detector.c" "handler.c" "main.c" "temperature.c" "mqtt.c handler.c main.c wifi.c mqtt.c dht11.c cJSON.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()
# component_compile_options(-Wno-error=format= -Wno-format)at)
component_compile_options(-Wno-error=format= -Wno-format)