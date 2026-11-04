//
// Created by haydn on 11/2/26.
//

#include <cstdint>
#include <array>
#include <cstring>
#include <string_view>
#include <ranges>

#include "esp32_pins.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "socket.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "../../shared/include/helpers.h"
#include "../../shared/include/rfm95w.h"
#include "rfm95w_esp32interface.h"
#include "secrets.h"
#include "driver/gpio.h"

constexpr uint32_t NT_STACK_SIZE = 4096;
static StaticTask_t xNetworkTaskBuffer;
static StackType_t xNetworkTaskStack[NT_STACK_SIZE];

constexpr uint32_t RT_STACK_SIZE = 4096;
static StaticTask_t xRadioTaskBuffer;
static StackType_t xRadioTaskStack[RT_STACK_SIZE];

struct sensorBundle {
    float temperature;
    float pressure;
    float humidity;
};

constexpr uint32_t QUEUE_LENGTH = 10;
constexpr uint32_t ITEM_SIZE    = sizeof(sensorBundle);
static StaticQueue_t xStaticQueue;
static uint8_t ucQueueStorageArea[QUEUE_LENGTH * ITEM_SIZE];
QueueHandle_t xQueue = xQueueCreateStatic(  QUEUE_LENGTH, ITEM_SIZE,
                                            ucQueueStorageArea, &xStaticQueue);

TaskHandle_t xNetworkTaskHandle = nullptr;
TaskHandle_t xRadioTaskHandle = nullptr;
// for error log
static auto TAG = "radio";

// check if wifi is on
static std::atomic isOnline{false};

static void wifiHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData)
{
    if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    } else if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_DISCONNECTED){
        isOnline = false;
    } else if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP)
    {
        isOnline = true;
        ESP_LOGI(TAG, "Got IP, Wifi connected");
    }
}

static void wifiSetup()
{
    nvs_flash_init();
    esp_netif_init();

    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t initCfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&initCfg);

    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifiHandler,
                                        nullptr,
                                        nullptr);
    esp_event_handler_instance_register(IP_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifiHandler,
                                        nullptr,
                                        nullptr);

    wifi_config_t cred{};
    std::ranges::copy(WIFI_SSID, cred.sta.ssid);
    std::ranges::copy(WIFI_PASS, cred.sta.password);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &cred);
    esp_wifi_start();
}

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

        const sensorBundle sensorReading{
            temp, pres, hum
        };
        xQueueSend(xQueue, &sensorReading, pdMS_TO_TICKS(5000));
    }
    vTaskDelete(nullptr);
}

void StartNetworkTask(void *pvParameters)
{
    // Setup
    wifiSetup();
    const auto sock = socket(AF_INET, SOCK_DGRAM, 0);
    constexpr int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    addr.sin_port = htons(WIFI_PORT);
    //bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));

    for (;;)
    {
        std::array<uint8_t, 12> txBuf{};
        if (xQueueReceive(xQueue, txBuf.data(), pdMS_TO_TICKS(1000)) != pdTRUE) continue;
        if (isOnline) {
            sendto(sock, txBuf.data(), txBuf.size(), 0, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
        }
    }
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

static void vCreateNetworkTask()
{
    xRadioTaskHandle = xTaskCreateStaticPinnedToCore(
      StartNetworkTask,
      "NETWORK TASK",
      NT_STACK_SIZE,
      nullptr,
      tskIDLE_PRIORITY + 1,
      xNetworkTaskStack,
      &xNetworkTaskBuffer,
      0);
}

extern "C" void app_main(void)
{
    vCreateRadioTask();
    vCreateNetworkTask();
}