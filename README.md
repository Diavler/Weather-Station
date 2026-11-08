# Weather Station
## Overview
This repo contains C++23 code for an STM32 446RE, ESP32, and some shared driver code. The goal is to collect weather data,
transmit it via LoRa, and then make it accessible with WiFi.
### STM32
On the STM32 side, there are drivers for a BME280 and BMP580 (environment sensors). 
The STM32 also implements an interface for an RFM95W (driver is in shared folder). The STM32's RFM95W
transmits the weather data over LoRa to the ESP32.
### ESP32
On the ESP32 side, there is an interface for an RFM95W. The ESP32 broadcasts the weather data received
from the STM32 on WiFi with UDP.
### FreeRTOS
FreeRTOS is used on the STM32 and ESP32 for concurrency, queues, task notifications, and interrupts.
## Todo
- Cleanup/refactoring
- Flesh out readme
## Building
Install:
- STM32CubeMX
- STM32CubeCLT
- arm-none-eabi-gcc
- arm-none-eabi-gdb
- ESP-IDF
