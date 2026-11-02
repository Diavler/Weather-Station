//
// Created by haydn on 10/29/26.
//

#ifndef WEATHERSTATION_RFM95W_CONSTANTS_H
#define WEATHERSTATION_RFM95W_CONSTANTS_H
#include <cstdint>

/* Start of Constants */

static constexpr uint8_t REG_OPMODE     = 0x01;
static constexpr uint8_t MODE_LORA      = 1u << 7; // bit 7 of opmode (long range mode)
static constexpr uint8_t MASK_MODE      = 0b111; // bits 2:0 of opmode (mode)
static constexpr uint8_t MODE_SLEEP     = 0b000;
static constexpr uint8_t MODE_STANDBY   = 0b001;
static constexpr uint8_t MODE_TX        = 0b011;
static constexpr uint8_t MODE_RXC       = 0b101;
static constexpr uint8_t MODE_RXS       = 0b110;
static constexpr uint8_t MODE_CAD       = 0b111;

// 915Mhz compensation
static constexpr uint8_t REG_FR_MSB     = 0x06;
static constexpr uint8_t REG_FR_MID     = 0x07;
static constexpr uint8_t REG_FR_LSB     = 0x08;
static constexpr uint8_t VAL_FR_MSB     = 0b11100100;
static constexpr uint8_t VAL_FR_MID     = 0b11000000;
static constexpr uint8_t VAL_FR_LSB     = 0b00000000;

// Config
static constexpr uint8_t REG_MCONF1     = 0x1D; // ModemConfig
static constexpr uint8_t REG_MCONF2     = 0x1E;
static constexpr uint8_t REG_MCONF3     = 0x26;
static constexpr uint8_t SET_CRC        = 1u << 2; // Bit 2 of ModemConfig2
static constexpr uint8_t REG_PACONF     = 0x09;
static constexpr uint8_t SET_PABOOST    = 1u << 7; // Bit 7 of PaConfig
static constexpr uint8_t REG_DIOMAP1    = 0x40;
static constexpr uint8_t MASK_DIO0      = 0xC0; // Bits 7:6 for DIO0
static constexpr uint8_t SET_DIO0TX     = 0b01 << 6; // TxDone
static constexpr uint8_t SET_DIO0RX     = 0b00 << 6; // RxDone

// FIFO (General)
static constexpr uint8_t MAX_PAYSIZE    = 0x20; // 32 bytes payload
static constexpr uint8_t REG_FIFO       = 0x00;
static constexpr uint8_t REG_FIFO_ADDR  = 0x0D;
static constexpr uint8_t REG_PAYLEN     = 0x22;
static constexpr uint8_t FIFO_TX_BASE   = 0x00;
static constexpr uint8_t FIFO_RX_BASE   = 0x80;

// IRQ Flags
static constexpr uint8_t REG_IRQ_FLAGS  = 0x12;
static constexpr uint8_t MASK_RXTIMEOUT = 1u << 7;
static constexpr uint8_t MASK_RXDONE    = 1u << 6;
static constexpr uint8_t MASK_PLCRCERR  = 1u << 5;
static constexpr uint8_t MASK_VALHEADER = 1u << 4;
static constexpr uint8_t MASK_TXDONE    = 1u << 3;

// FIFO (Rx)
static constexpr uint8_t REG_RXNBBYTES  = 0x13;
static constexpr uint8_t REG_FIFO_RX_CURR_ADDR = 0x10;
static constexpr uint8_t REG_FIFO_RX_BYTE_ADDR = 0x25;
static constexpr uint8_t REG_RX_NB_BYTES = 0x13;

/* End of Constants */

#endif //WEATHERSTATION_RFM95W_CONSTANTS_H
