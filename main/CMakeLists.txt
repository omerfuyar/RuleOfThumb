idf_component_register(SRCS "main.c" "lcd.c" "input.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_lcd esp_driver_spi esp_driver_gpio esp_timer)

target_compile_options(${COMPONENT_LIB} PRIVATE -fanalyzer)
