//
// Created by haydn on 11/2/26.
//

#ifndef ESP32_STATION_ESP32_PINS_H
#define ESP32_STATION_ESP32_PINS_H
#include <cstdint>
#include "driver/gpio.h"

constexpr gpio_num_t PIN_RST   = GPIO_NUM_4;
constexpr gpio_num_t PIN_G0  = GPIO_NUM_22;
constexpr gpio_num_t PIN_MOSI  = GPIO_NUM_23;
constexpr gpio_num_t PIN_MISO  = GPIO_NUM_19;
constexpr gpio_num_t PIN_SCK  = GPIO_NUM_18;
constexpr gpio_num_t PIN_CS    = GPIO_NUM_5;

#endif //ESP32_STATION_ESP32_PINS_H
