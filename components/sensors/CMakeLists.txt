idf_component_register(SRCS "spi.c" "bme280.c" "sensors.c"
                       INCLUDE_DIRS "include"
                       REQUIRES common mqtt esp_driver_spi)
