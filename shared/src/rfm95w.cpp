//
// Created by Haydn on 10/10/2026.
//

#include "../include/rfm95w.h"
#include "../include/rfm95w_constants.h"
#include <array>
/* Helper Functions */

/// Read a register
/// @param addr
/// @return
std::expected<uint8_t, ErrorCode> RFM95W::readRegister(const uint8_t addr) const
{
    constexpr auto spiSize = 2;
    std::array<uint8_t, spiSize> tx{static_cast<uint8_t>(addr & 0x7F), 0x00};
    std::array<uint8_t, spiSize> rx{0,0};

    if (const auto err = m_interface.spiTransfer(tx, rx); err != ErrorCode::Success) return std::unexpected(err);
    return rx[1];
}

/// Overwrite a register
/// @param addr
/// @param val
/// @return
ErrorCode RFM95W::writeRegister(const uint8_t addr, const uint8_t val) {
    std::array tx{static_cast<uint8_t>(addr | 0x80), val};

    if (const auto err = m_interface.spiTransfer(tx, {}); err != ErrorCode::Success) return err;
    return ErrorCode::Success;
}

/// Update part of a register with a mask and value
/// @param addr
/// @param mask
/// @param val
/// @return
ErrorCode RFM95W::updateRegister(const uint8_t addr, const uint8_t mask, const uint8_t val) {
    auto read = readRegister(addr);
    if (!read) return read.error();
    // Mask the register, then set the new part of the register
    const auto newVal = (*read & ~mask) | (val & mask);
    return writeRegister(addr, newVal);
}

/* End of helper functions */

/// Set RFM95W to LoRa mode at 915MHz with CRC on
/// @return
ErrorCode RFM95W::boot() {
    m_interface.reset();

    // Enable LoRa + Standby
    static constexpr std::array<RegUpdate, 3> updateOPMODE {{
        {.addr = REG_OPMODE, .mask = MASK_MODE, .val = MODE_SLEEP},
        {.addr = REG_OPMODE, .mask = MODE_LORA, .val = MODE_LORA},
        {.addr = REG_OPMODE, .mask = MASK_MODE, .val = MODE_STANDBY}
    }};
    for (const auto& [addr, mask, val] : updateOPMODE) {
        if (const auto err = updateRegister(addr, mask, val); err != ErrorCode::Success) return err;
    }
    // Set Frequency to 915MHz
    static constexpr std::array<RegWrite, 3> setFREQ {{
        {.addr = REG_FR_MSB, .val = VAL_FR_MSB},
        {.addr = REG_FR_MID, .val = VAL_FR_MID},
        {.addr = REG_FR_LSB, .val = VAL_FR_LSB},
    }};
    for (const auto& [addr, val] : setFREQ) {
        if (const auto err = writeRegister(addr, val); err != ErrorCode::Success) return err;
    }
    static constexpr std::array<RegUpdate, 3> setMISC {{
        {.addr = REG_MCONF2, .mask = SET_CRC, .val = SET_CRC},          // RxPayloadCrcOn to 1
        {.addr = REG_PACONF, .mask = SET_PABOOST, .val = SET_PABOOST},  // PaSelect to PA_BOOST pin
    }};
    for (const auto& [addr, mask, val] : setMISC) {
        if (const auto err = updateRegister(addr, mask, val); err != ErrorCode::Success) return err;
    }

    return ErrorCode::Success;
}

/// Transmit a buffer with len bytes
/// @param buf data to transmit
/// @return
ErrorCode RFM95W::transmit(std::span<uint8_t> buf) {
    // Clear all IRQ flags
    if (const auto err = writeRegister(REG_IRQ_FLAGS, 0xFF); err != ErrorCode::Success) return err;
    // Map DIO0 --> TxDone
    if (const auto err = updateRegister(REG_DIOMAP1, MASK_DIO0, SET_DIO0TX); err != ErrorCode::Success) return err;
    // Set FifoPtrAddr --> FifoTxPtrBase
    if (const auto err = writeRegister(REG_FIFO_ADDR, FIFO_TX_BASE); err != ErrorCode::Success) return err;

    // Assemble buffer
    std::array<uint8_t, MAX_PAYSIZE + 1> tx{};
    tx[0] = static_cast<uint8_t>(REG_FIFO | 0x80);
    std::copy(buf.begin(), buf.end(), tx.begin()+1);

    // Set TX mode
    if (const auto err = updateRegister(REG_OPMODE, MASK_MODE, MODE_TX); err != ErrorCode::Success) return err;
    // Set Payload
    if (const auto err = writeRegister(REG_PAYLEN, tx.size()-1); err != ErrorCode::Success) return err;
    // Transmit
    if (const auto err = m_interface.spiTransfer(tx, {}); err != ErrorCode::Success) return err;

    return ErrorCode::Success;
}

/// Clear TxDone after an interrupt
/// @return
ErrorCode RFM95W::clearIRQ_txDone() {
    return writeRegister(REG_IRQ_FLAGS, MASK_TXDONE);
}

/// Setup for continuous transmission
/// @return
ErrorCode RFM95W::beginReceive() {
    // Map DIO0 --> RxDone
    if (const auto err = updateRegister(REG_DIOMAP1, MASK_DIO0, SET_DIO0RX); err != ErrorCode::Success) return err;
    // Mode --> RX Continuous
    if (const auto err = updateRegister(REG_OPMODE, MASK_MODE, MODE_RXC); err != ErrorCode::Success) return err;

    return ErrorCode::Success;
}

/// Check IRQ flags and read FIFO into buf. Call beginReceive() before.
/// @param buf buffer to read into
/// @return
ErrorCode RFM95W::getPayload(std::span<uint8_t> buf) {
    // Check the IRQ
    auto irqFlags = readRegister(REG_IRQ_FLAGS);
    if (!irqFlags) return irqFlags.error();
    if (!(*irqFlags | MASK_RXDONE))     return ErrorCode::badPacket;
    if (*irqFlags | MASK_PLCRCERR)      return ErrorCode::badPacket;
    constexpr uint8_t setIRQ = MASK_RXDONE | MASK_RXTIMEOUT | MASK_PLCRCERR | MASK_VALHEADER;
    if (const auto err = writeRegister(REG_IRQ_FLAGS, setIRQ); err != ErrorCode::Success) return err;

    // Get amount of bytes
    const auto fifoRxBytesNb = readRegister(REG_RX_NB_BYTES);
    if (!fifoRxBytesNb) return ErrorCode::spiError;
    if (*fifoRxBytesNb > MAX_PAYSIZE) return ErrorCode::spiError;

    // Get the location of the packet
    const auto fifoRxCurrentAddr = readRegister(REG_FIFO_RX_CURR_ADDR);
    if (!fifoRxCurrentAddr) return ErrorCode::spiError;

    // Set FifoPtrAddr --> FifoRxCurrentAddr
    if (const auto err = writeRegister(REG_FIFO_ADDR, *fifoRxCurrentAddr); err != ErrorCode::Success) return err;
    // Temporary buffers
    constexpr auto spiSize = MAX_PAYSIZE + 1;
    std::array<uint8_t, spiSize> tx{REG_FIFO & 0x7F};
    std::array<uint8_t, spiSize> rx{};
    // Make a view with the correct size to read into
    const auto viewTx = std::span(tx).first(*fifoRxBytesNb + 1);
    const auto viewRx = std::span(rx).first(*fifoRxBytesNb + 1);

    if (const auto err = m_interface.spiTransfer(viewTx, viewRx); err != ErrorCode::Success) return err;
    std::copy(rx.begin(), rx.end(), buf.begin());

    return ErrorCode::Success;
}
