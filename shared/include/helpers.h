//
// Created by Haydn on 8/14/2026.
//

#ifndef WEATHERSTATION_HELPERS_H
#define WEATHERSTATION_HELPERS_H
#include <cstdint>

/// Generic error codes
enum class [[nodiscard]] ErrorCode : uint8_t {
    Success,
    spiError,
    i2cError,
    uartError,
    timeout,
    invalidArgs,
    badPacket,
};

/// Convert generic error codes to strings
constexpr const char8_t* errorString(const ErrorCode error) {
    switch (error) {
        case ErrorCode::Success:            return u8"Success\r\n";
        case ErrorCode::spiError:           return u8"SPI Failure\r\n";
        case ErrorCode::i2cError:           return u8"I2C Failure\r\n";
        case ErrorCode::uartError:          return u8"UART Failure\r\n";
        case ErrorCode::timeout:            return u8"Timeout\r\n";
        case ErrorCode::invalidArgs:        return u8"Invalid arguments\r\n";
        case ErrorCode::badPacket:          return u8"Bad packet\r\n";
        default:                            return u8"ERROR: UNKNOWN\r\n";
    }
}

#endif //WEATHERSTATION_HELPERS_H
