set(COMPONENT_SRCS "main.c wifi.c mqtt.c dht11.c cjson.c handler.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()
component_compile_options(-Wno-error=format= -Wno-format)