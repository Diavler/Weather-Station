//
// Created by Haydn on 10/10/2026.
//

#ifndef WEATHERSTATION_RFM95W_H
#define WEATHERSTATION_RFM95W_H

#include "helpers.h"
#include <span>
#include <expected>

#include "rfm95w_constants.h"
#include "rfm95w_interface.h"

/// RFM95W LoRa driver
class RFM95W {
public:
    explicit RFM95W(RFM95WInterface& interface) : m_interface(interface) {}
    ErrorCode boot();
    ErrorCode beginReceive();
    ErrorCode transmit(std::span<uint8_t> buf);
    ErrorCode getPayload(std::span<uint8_t> buf);
    ErrorCode clearIRQ_txDone();
private:
    RFM95WInterface& m_interface;
    [[nodiscard]] std::expected<uint8_t, ErrorCode>  readRegister(uint8_t addr) const;
    ErrorCode writeRegister(uint8_t addr, uint8_t val);
    ErrorCode updateRegister(uint8_t addr, uint8_t mask, uint8_t val);
};

struct RegWrite
{
    uint8_t addr;
    uint8_t val;
};

struct RegUpdate
{
    uint8_t addr;
    uint8_t mask;
    uint8_t val;
};

#endif //WEATHERSTATION_RFM95W_H
