idf_component_register(
    SRCS "main_simple.c" "main.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer freertos
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format)
