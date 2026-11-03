//
// Created by Haydn on 8/14/2026.
//

#ifndef WEATHERSTATION_HELPERS_H
#define WEATHERSTATION_HELPERS_H
#include <cstdint>
#include <string_view>

/// Generic error codes
enum class [[nodiscard]] ErrorCode : uint8_t {
    Success,
    spiError,
    i2cError,
    uartError,
    gpioError,
    timeout,
    invalidArgs,
    badPacket,
    crcError,
    invalidLength,
    badDevice,
};

/// Convert generic error codes to strings
constexpr std::string_view errorString(const ErrorCode error) {
    switch (error) {
        case ErrorCode::Success:            return "Success\r\n";
        case ErrorCode::spiError:           return "SPI Failure\r\n";
        case ErrorCode::i2cError:           return "I2C Failure\r\n";
        case ErrorCode::uartError:          return "UART Failure\r\n";
        case ErrorCode::timeout:            return "Timeout\r\n";
        case ErrorCode::invalidArgs:        return "Invalid arguments\r\n";
        case ErrorCode::badPacket:          return "Bad packet\r\n";
        case ErrorCode::gpioError:          return "GPIO failure\r\n";
        case ErrorCode::crcError:           return "CRC Error\r\n";
        case ErrorCode::invalidLength:      return "Invalid length\r\n";
        case ErrorCode::badDevice:          return "Bad device\r\n";
        default:                            return "ERROR: UNKNOWN\r\n";
    }
}

#endif //WEATHERSTATION_HELPERS_H
