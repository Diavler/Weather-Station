//
// Created by haydn on 11/2/26.
//

#include <cstdint>
#include <array>
#include <cstring>
#include <string_view>

#include "esp32_pins.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "../../shared/include/helpers.h"
#include "../../shared/include/rfm95w.h"
#include "rfm95w_esp32interface.h"
#include "driver/gpio.h"

//constexpr uint32_t ST_STACK_SIZE = 4096;
//static StaticTask_t xNetworkTaskBuffer;
//static StackType_t xNetworkTaskStack[ST_STACK_SIZE];

constexpr uint32_t RT_STACK_SIZE = 4096;
static StaticTask_t xRadioTaskBuffer;
static StackType_t xRadioTaskStack[RT_STACK_SIZE];

//TaskHandle_t xNetworkTaskHandle = nullptr;
TaskHandle_t xRadioTaskHandle = nullptr;

static auto TAG = "radio";

void StartRadioTask(void *pvParameters)
{
    static RFM95WInterfaceESP32 radioInterface(xTaskGetCurrentTaskHandle());
    static RFM95W radio(radioInterface);
    if (auto err = radio.boot(); err != ErrorCode::Success) {
        auto msg = errorString(err);
        ESP_LOGE(TAG, "radio boot failed: %.*s", static_cast<int>(msg.size()), msg.data());
        vTaskDelete(nullptr);
    }

    if (radio.beginReceive() !=  ErrorCode::Success) {
        ESP_LOGE(TAG, "Error transitioning radio to continuous RX");
    }

    for (;;) {
        std::array<uint8_t, 32> rx{};
        auto payloadLength = radio.getPayload(rx, 10000);
        if (!payloadLength) {
            if (payloadLength.error() == ErrorCode::timeout) {
                if (const auto r = radio.debugRegs()) {
                    ESP_LOGI(TAG, "timeout: G0=%d flags=0x%02X opmode=0x%02X dio=0x%02X ver=0x%02X",
                             gpio_get_level(PIN_G0), r->irqFlags, r->opMode, r->dioMap1, r->version);
                }
            } else {
                auto msg = errorString(payloadLength.error());
                ESP_LOGE(TAG, "Error getting payload: %.*s", static_cast<int>(msg.size()), msg.data());
            }
            continue;
        }
        float temp, pres, hum;
        auto payload = std::span<const uint8_t>(rx).first(*payloadLength);
        std::memcpy(&temp, payload.data(), sizeof(float));
        std::memcpy(&pres, payload.data() + 4, sizeof(float));
        std::memcpy(&hum, payload.data() + 8, sizeof(float));

        ESP_LOGI(TAG, "%.2f C   %.2f kPa    %.1f %% Humidity", temp, pres, hum);
    }
    vTaskDelete(nullptr);
}

static void vCreateRadioTask()
{
    xRadioTaskHandle = xTaskCreateStaticPinnedToCore(
      StartRadioTask,
      "RADIO TASK",
      RT_STACK_SIZE,
      nullptr,
      tskIDLE_PRIORITY + 2,
      xRadioTaskStack,
      &xRadioTaskBuffer,
      1);
}

extern "C" void app_main(void)
{
    vCreateRadioTask();
}