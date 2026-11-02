//
// Created by haydn on 10/29/26.
//

#ifndef WEATHERSTATION_RFM95W_INTERFACE_H
#define WEATHERSTATION_RFM95W_INTERFACE_H
#include <cstdint>
#include <expected>
#include <span>

#include "helpers.h"

/// The ESP32 and STM32 implement these functions

class RFM95WInterface
{
    public:
        RFM95WInterface(const RFM95WInterface&) = delete;
        RFM95WInterface& operator=(const RFM95WInterface&) = delete;
        /// HAL Implementation for SPI
        /// @param tx transmit buffer
        /// @param rx receive buffer
        /// @return
        virtual ErrorCode spiTransfer(std::span<const uint8_t> tx, std::span<uint8_t> rx) = 0;
        /// Toggle the reset pin
        virtual void reset() = 0;

    protected:
        RFM95WInterface() = default;
        ~RFM95WInterface() = default;
    private:
        virtual void resetCS() = 0;
        virtual void setCS() = 0;

};

#endif //WEATHERSTATION_RFM95W_INTERFACE_H
