//
// Created by haydn on 11/2/26.
//

#ifndef ESP32_STATION_RFM95W_ESP32INTERFACE_H
#define ESP32_STATION_RFM95W_ESP32INTERFACE_H

#include "../../shared/include/helpers.h"
#include "../../shared/include/rfm95w_interface.h"
#include <cstdint>
#include <atomic>
#include <span>
#include <bits/shared_ptr_atomic.h>

#include "driver/spi_master.h"
#include "freertos/task.h"

class RFM95WInterfaceESP32 final : public RFM95WInterface
{
    public:
        ~RFM95WInterfaceESP32() = default;
        explicit RFM95WInterfaceESP32(TaskHandle_t waitingTask)
            : RFM95WInterface(), m_waitingTask(waitingTask) {};
        ErrorCode init() override;
        ErrorCode spiTransfer(std::span<const uint8_t> tx, std::span<uint8_t> rx) override;
        void reset() override;
        void onDIO0Interrupt() override;
        void setIRQ() override;
        bool waitForIRQ(uint32_t timeoutMs) override;
    private:
        static void dio0isr(void* arg);
        spi_device_handle_t m_spiPtr = nullptr;
        std::atomic<TaskHandle_t> m_waitingTask {nullptr};

};
#endif //ESP32_STATION_RFM95W_ESP32INTERFACE_H
