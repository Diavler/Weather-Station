//
// Created by haydn on 11/2/26.
//

#include "esp32_pins.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "portmacro.h"
#include "rfm95w_esp32interface.h"

ErrorCode RFM95WInterfaceESP32::spiTransfer(std::span<const uint8_t> tx, std::span<uint8_t> rx)
{
    if (m_spiPtr == nullptr) return ErrorCode::invalidArgs;
    if (tx.empty() || tx.size() > 64) return ErrorCode::invalidArgs;
    if (!rx.empty() && rx.size() != tx.size()) return ErrorCode::invalidArgs;

    spi_transaction_t transaction = {};
    transaction.length = 8 * tx.size();
    transaction.tx_buffer = tx.data();
    transaction.rx_buffer = rx.empty() ? nullptr : rx.data();

    if (spi_device_polling_transmit(m_spiPtr, &transaction) != ESP_OK) return ErrorCode::spiError;
    return ErrorCode::Success;
}

ErrorCode RFM95WInterfaceESP32::init()
{
    // GPIO: rst, dio0/g0 13-14
    gpio_config_t rst{};
    rst.pin_bit_mask = 1ULL << PIN_RST;
    rst.mode = GPIO_MODE_OUTPUT;
    rst.pull_up_en = GPIO_PULLUP_DISABLE;
    rst.pull_down_en = GPIO_PULLDOWN_DISABLE;
    rst.intr_type = GPIO_INTR_DISABLE;

    if (gpio_config(&rst) != ESP_OK) return ErrorCode::gpioError;
    if (gpio_set_level(PIN_RST, 1)!= ESP_OK) return ErrorCode::gpioError;

    gpio_config_t dio0{};
    dio0.pin_bit_mask = 1ULL << PIN_G0;
    dio0.mode = GPIO_MODE_INPUT;
    dio0.pull_up_en = GPIO_PULLUP_DISABLE;
    dio0.pull_down_en = GPIO_PULLDOWN_ENABLE;
    dio0.intr_type = GPIO_INTR_POSEDGE;

    if (gpio_config(&dio0) != ESP_OK) return ErrorCode::gpioError;

    // SPI: pins 25 - 27
    spi_bus_config_t bus{};
    bus.mosi_io_num = PIN_MOSI;
    bus.miso_io_num = PIN_MISO;
    bus.sclk_io_num = PIN_SCK;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = 64;
    bus.data4_io_num = -1;
    bus.data5_io_num = -1;
    bus.data6_io_num = -1;
    bus.data7_io_num = -1;

    if (spi_bus_initialize(SPI3_HOST, &bus, SPI_DMA_DISABLED) != ESP_OK) return ErrorCode::spiError;

    spi_device_interface_config_t dev{};
    dev.mode = 0;
    dev.clock_speed_hz = 5'000'000;
    dev.spics_io_num = PIN_CS;
    dev.queue_size = 1;

    if (spi_bus_add_device(SPI3_HOST, &dev, &m_spiPtr) != ESP_OK) return ErrorCode::spiError;
    gpio_set_pull_mode(PIN_MISO, GPIO_PULLUP_ONLY);

    if (gpio_install_isr_service(0) != ESP_OK) return ErrorCode::gpioError;
    if (gpio_isr_handler_add(PIN_G0, dio0isr, this) != ESP_OK) return ErrorCode::gpioError;

    return ErrorCode::Success;
}

void RFM95WInterfaceESP32::reset()
{
    gpio_set_level(PIN_RST, 0);
    esp_rom_delay_us(1 * 1000);
    gpio_set_level(PIN_RST, 1);
    esp_rom_delay_us(10 * 1000);

}

void RFM95WInterfaceESP32::dio0isr(void* arg)
{
    auto* self =  static_cast<RFM95WInterfaceESP32*>(arg);
    self->onDIO0Interrupt();
}

void RFM95WInterfaceESP32::setIRQ()
{
    m_waitingTask = xTaskGetCurrentTaskHandle();
    ulTaskNotifyTake(pdTRUE, 0);
}

bool RFM95WInterfaceESP32::waitForIRQ(const uint32_t timeoutMs)
{
    return ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeoutMs) + 1) > 0;
}

void RFM95WInterfaceESP32::onDIO0Interrupt()
{
    const auto task = m_waitingTask.load();
    if (task == nullptr) return;
    BaseType_t taskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(task, &taskWoken);
    portYIELD_FROM_ISR(taskWoken);
}