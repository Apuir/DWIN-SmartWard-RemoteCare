idf_component_register(
    SRCS "src/max98357_i2s.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_i2s esp_driver_gpio
)
