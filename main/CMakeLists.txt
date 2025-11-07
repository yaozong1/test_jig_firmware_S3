idf_component_register(
    SRCS "main.c" "voltage_adc.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer freertos
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format)
