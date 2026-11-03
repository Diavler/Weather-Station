//
// Created by haydn on 10/30/26.
//

#ifndef WEATHERSTATION_RFM95W_STM32INTERFACE_H
#define WEATHERSTATION_RFM95W_STM32INTERFACE_H

#include <../../../shared/include/rfm95w_interface.h>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

class RFM95WInterfaceSTM32 final : public RFM95WInterface
{
    public:
        ~RFM95WInterfaceSTM32() = default;
        explicit RFM95WInterfaceSTM32
            (SPI_HandleTypeDef* ptr, GPIO_TypeDef* csPort, uint16_t csPin, GPIO_TypeDef* rstPort, uint16_t rstPin)
            : RFM95WInterface(), m_hspiPtr(ptr), m_csPort(csPort), m_csPin(csPin), m_rstPort(rstPort), m_rstPin(rstPin) {};

        ErrorCode spiTransfer(std::span<const uint8_t> tx, std::span<uint8_t> rx) override;
        void reset() override;
        void onDIO0Interrupt() override;
        void setIRQ() override;
        bool waitForIRQ(uint32_t timeoutMs) override;
    private:
        SPI_HandleTypeDef* m_hspiPtr = nullptr;
        void resetCS() { HAL_GPIO_WritePin(m_csPort, m_csPin, GPIO_PIN_RESET); }
        void setCS() { HAL_GPIO_WritePin(m_csPort, m_csPin, GPIO_PIN_SET); }
        GPIO_TypeDef* m_csPort;
        uint16_t m_csPin;
        GPIO_TypeDef* m_rstPort;
        uint16_t m_rstPin;
        volatile TaskHandle_t m_waitingTask = nullptr;


};
#endif //WEATHERSTATION_RFM95W_STM32INTERFACE_H
