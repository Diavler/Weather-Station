//
// Created by Haydn on 8/13/2026.
//
#include "main_ws.h"

#include <array>
#include <cstdio>
#include <cstring>

#include "bme280.h"
#include "bmp580.h"
#include "../../../shared/include/rfm95w.h"
#include "rfm95w_stm32interface.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stm32f4xx_hal.h"

/// SETUP

constexpr uint32_t ST_STACK_SIZE = 2048;
static StaticTask_t xSensorTaskBuffer;
static StackType_t xSensorTaskStack[ST_STACK_SIZE];

constexpr uint32_t RT_STACK_SIZE = 1024;
static StaticTask_t xRadioTaskBuffer;
static StackType_t xRadioTaskStack[RT_STACK_SIZE];

TaskHandle_t xSensorTaskHandle = nullptr;
TaskHandle_t xRadioTaskHandle = nullptr;

static void vCreateSensorTask() {
    xSensorTaskHandle = xTaskCreateStatic(
        StartSensorTask,
        "SENSOR TASK",
        ST_STACK_SIZE,
        nullptr,
        tskIDLE_PRIORITY + 1,
        xSensorTaskStack,
        &xSensorTaskBuffer );
}


static void vCreateRadioTask() {
    xRadioTaskHandle = xTaskCreateStatic(
        StartRadioTask,
        "RADIO TASK",
        RT_STACK_SIZE,
        nullptr,
        tskIDLE_PRIORITY + 2,
        xRadioTaskStack,
        &xRadioTaskBuffer );
}

constexpr uint32_t QUEUE_LENGTH = 10;
constexpr uint32_t ITEM_SIZE    = sizeof(sensorBundle);
static StaticQueue_t xStaticQueue;
static uint8_t ucQueueStorageArea[QUEUE_LENGTH * ITEM_SIZE];
QueueHandle_t xQueue = xQueueCreateStatic(  QUEUE_LENGTH, ITEM_SIZE,
                                            ucQueueStorageArea, &xStaticQueue);

/* Import handles from STM32 generated code */
extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart2;

/// TASKS

void main_ws() {
    vCreateSensorTask();
    vCreateRadioTask();

    vTaskStartScheduler();
}

/// Temporary function to clean up errors. Todo: implement more elegant string / error handling
static void sendUARTErrorCode(ErrorCode err) {
    const auto msg = errorString(err);
    HAL_UART_Transmit(&huart2, reinterpret_cast<const uint8_t *>(msg), strlen(reinterpret_cast<const char *>(msg)), HAL_MAX_DELAY);
}

void StartSensorTask(void *pvParameters) {
    // TODO: implement cleaner error codes + handling
    if (BMP580::start(&hi2c1) == ErrorCode::i2cError) {
        char startFailMsg[] = "ERROR: BMP580 failed to start.\r\n";
        HAL_UART_Transmit(&huart2, reinterpret_cast<uint8_t*>(startFailMsg), strlen(startFailMsg), HAL_MAX_DELAY);
        vTaskDelete(nullptr);
    } else {
        char startMsg[] = "BMP580 powered on!\r\n";
        HAL_UART_Transmit(&huart2, reinterpret_cast<uint8_t*>(startMsg), strlen(startMsg), HAL_MAX_DELAY);
    }

    if (BME280::start(&hi2c1) == ErrorCode::i2cError) {
        char startFailMsg[] = "ERROR: BME280 failed to start.\r\n";
        HAL_UART_Transmit(&huart2, reinterpret_cast<uint8_t*>(startFailMsg), strlen(startFailMsg), HAL_MAX_DELAY);
        vTaskDelete(nullptr);
    } else {
        char startMsg[] = "BME280 powered on!\r\n";
        HAL_UART_Transmit(&huart2, reinterpret_cast<uint8_t*>(startMsg), strlen(startMsg), HAL_MAX_DELAY);
    }

    for(;;)
    {
        const auto tempRet = BMP580::readTemperature();
        const auto presRet = BMP580::readPressure();
        const auto humRet   = BME280::readHumidity();

        for (ErrorCode err : {tempRet, presRet, humRet}) {
            if (err == ErrorCode::i2cError) {sendUARTErrorCode(ErrorCode::i2cError);}
        }

        const sensorBundle sensorReading{
            BMP580::getTemperature(), BMP580::getPressure(), BME280::getHumidity()
        };
        char output[64];
        const int len = snprintf(output, sizeof(output), "%.2f C    %.2f kPa    %.1f%% Humidity\r\n",
            sensorReading.temperature, sensorReading.pressure, sensorReading.humidity);
        HAL_UART_Transmit(&huart2, reinterpret_cast<uint8_t*>(output), len, HAL_MAX_DELAY);

        xQueueSend(xQueue, &sensorReading, pdTICKS_TO_MS(5000));
        xTaskNotifyGive(xRadioTaskHandle);
        vTaskDelay(pdTICKS_TO_MS(5000));
    }
    vTaskDelete(nullptr);
}

void StartRadioTask(void *pvParameters) {
    vTaskDelay(500 / portTICK_RATE_MS);
    static RFM95WInterfaceSTM32 radioInterface(&hspi1, GPIOB, GPIO_PIN_6, GPIOC, GPIO_PIN_7);
    static RFM95W radio(radioInterface);
    if (const auto start_err = radio.boot(); start_err != ErrorCode::Success) {
        sendUARTErrorCode(start_err);
    };
    BaseType_t taskWoken = pdTRUE;

    for (;;) {
        constexpr TickType_t notifyTICKS = 250;
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        xTaskNotifyStateClear(xRadioTaskHandle);
        std::array<uint8_t, 32> txBuf{};
        xQueueReceive(xQueue, txBuf.data(), pdMS_TO_TICKS(1000));
        HAL_UART_Transmit(&huart2, txBuf.data(), txBuf.size(), HAL_MAX_DELAY);
        if (const auto transmitRet = radio.transmit(txBuf); transmitRet != ErrorCode::Success) {
            sendUARTErrorCode(transmitRet);
        }
        ulTaskNotifyTake(taskWoken, notifyTICKS);
        if (const auto clearRet = radio.clearIRQ_txDone(); clearRet != ErrorCode::Success) {
            sendUARTErrorCode(clearRet);
        }
    }
    vTaskDelete(nullptr);
}

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_9 && xRadioTaskHandle != nullptr) {
        BaseType_t taskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(xRadioTaskHandle, &taskWoken);
        portYIELD_FROM_ISR(taskWoken);
    }
}

