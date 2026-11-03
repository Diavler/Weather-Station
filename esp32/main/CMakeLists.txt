idf_component_register(SRCS "main.cpp" "rfm95w_esp32interface.cpp" "esp32_pins.h" "../../shared/src/rfm95w.cpp"
                    INCLUDE_DIRS "." "../../shared/include"
                    REQUIRES esp_driver_gpio esp_driver_spi)
