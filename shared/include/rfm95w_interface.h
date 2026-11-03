//
// Created by haydn on 10/29/26.
//

#ifndef WEATHERSTATION_RFM95W_INTERFACE_H
#define WEATHERSTATION_RFM95W_INTERFACE_H
#include <cstdint>
#include <span>

#include "helpers.h"

/// The ESP32 and STM32 implement these functions

class RFM95WInterface
{
    public:
        RFM95WInterface(const RFM95WInterface&) = delete;
        RFM95WInterface& operator=(const RFM95WInterface&) = delete;
        /// Optional init function to setup pins, etc
        /// @return
        virtual ErrorCode init() { return ErrorCode::Success; }
        /// HAL Implementation for SPI
        /// @param tx transmit buffer
        /// @param rx receive buffer
        /// @return
        virtual ErrorCode spiTransfer(std::span<const uint8_t> tx, std::span<uint8_t> rx) = 0;
        /// Toggle the reset pin
        virtual void reset() = 0;
        /// Resets the IRQ notification
        virtual void setIRQ() = 0;
        /// Blocks until an IRQ notification
        /// @return Whether or not there was an IRQ (false: timeout)
        virtual bool waitForIRQ(uint32_t timeoutMs) = 0;
        /// Fires on DIO0 Interrupt
        virtual void onDIO0Interrupt() = 0;
    protected:
        RFM95WInterface() = default;
        ~RFM95WInterface() = default;
};

#endif //WEATHERSTATION_RFM95W_INTERFACE_H
