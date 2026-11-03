//
// Created by haydn on 10/30/26.
//

#include "rfm95w_stm32interface.h"
#include "FreeRTOS.h"
#include "task.h"
#include "../../../../../.espressif/tools/xtensa-esp-elf/esp-15.2.0_20251204/xtensa-esp-elf/xtensa-esp-elf/include/c++/15.2.0/bits/atomic_wait.h"

ErrorCode
RFM95WInterfaceSTM32::spiTransfer(const std::span<const uint8_t> tx, std::span<uint8_t> rx)
{
    if (m_hspiPtr == nullptr) return ErrorCode::invalidArgs;
    if (tx.empty()) return ErrorCode::invalidArgs;
    if (!rx.empty() && rx.size() != tx.size()) return ErrorCode::invalidArgs;
    resetCS();
    const auto halStatus = rx.empty()
        ? HAL_SPI_Transmit(m_hspiPtr, tx.data(), tx.size(), HAL_MAX_DELAY)
        : HAL_SPI_TransmitReceive(m_hspiPtr, tx.data(), rx.data(), tx.size(), HAL_MAX_DELAY);
    setCS();

    return (halStatus == HAL_OK) ? ErrorCode::Success : ErrorCode::spiError;
}

void RFM95WInterfaceSTM32::reset() {
    HAL_GPIO_WritePin(m_rstPort, m_rstPin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(m_rstPort, m_rstPin, GPIO_PIN_SET);
    HAL_Delay(10);
}

void RFM95WInterfaceSTM32::setIRQ()
{
    m_waitingTask = xTaskGetCurrentTaskHandle();
    ulTaskNotifyTake(pdTRUE, 0);
}

bool RFM95WInterfaceSTM32::waitForIRQ(const uint32_t timeoutMs)
{
    return ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeoutMs) + 1) > 0;
}

void RFM95WInterfaceSTM32::onDIO0Interrupt()
{
    const auto task = m_waitingTask;
    if (task == nullptr) return;
    BaseType_t taskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(task, &taskWoken);
    portYIELD_FROM_ISR(taskWoken);
}
