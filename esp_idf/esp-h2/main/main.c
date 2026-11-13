#include <stdio.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ble_adv.h"
#include "bthome_v2.h"

#define TAG                     "app_main"
#define DEVICE_NAME             "H2-Sim"
#define BROADCAST_INTERVAL_MS   (10000)
#define SENSOR_TASK_STACK_SIZE  (4096)
#define SENSOR_TASK_PRIORITY    (5)

/**
 * @brief Generate simulated sensor readings
 * Replace this function with actual I2C/SPI sensor drivers when hardware is attached.
 */
static void read_sensors(bthome_sensor_data_t *data)
{
    static uint8_t s_packet_id = 0;

    data->packet_id = ++s_packet_id;
    /* Simulate temperature: 20.00 °C to 35.00 °C (2000 to 3500 centidegrees) */
    data->temperature_centi = 2000 + (int16_t)(esp_random() % 1501);
    /* Simulate humidity: 40.00 % to 80.00 % (4000 to 8000 centipercent) */
    data->humidity_centi    = 4000 + (uint16_t)(esp_random() % 4001);
}

/**
 * @brief FreeRTOS task responsible for periodic sensor reading and BLE broadcasting
 */
static void sensor_broadcast_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensor broadcast task started (Interval: %u ms)", BROADCAST_INTERVAL_MS);

    uint8_t adv_buffer[31];
    size_t adv_len = 0;
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1) {
        bthome_sensor_data_t sensor_data;
        read_sensors(&sensor_data);

        esp_err_t err = bthome_v2_build_adv_packet(DEVICE_NAME,
                                                   &sensor_data,
                                                   adv_buffer,
                                                   sizeof(adv_buffer),
                                                   &adv_len);
        if (err == ESP_OK) {
            err = ble_adv_update_payload(adv_buffer, (uint8_t)adv_len);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Packet #%u broadcasted | Temp: %.2f °C | Humi: %.2f %%",
                         sensor_data.packet_id,
                         sensor_data.temperature_centi / 100.0f,
                         sensor_data.humidity_centi / 100.0f);
            } else {
                ESP_LOGE(TAG, "Failed to update BLE advertising: %s", esp_err_to_name(err));
            }
        } else {
            ESP_LOGE(TAG, "Failed to build BTHome packet: %s", esp_err_to_name(err));
        }

        /* Periodic execution without drift */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(BROADCAST_INTERVAL_MS));
    }
}

/**
 * @brief Application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Starting BTHome v2 Sensor Beacon on ESP32-H2...");

    /* 1. Initialize Non-Volatile Storage (NVS) */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 2. Initialize NimBLE Bluetooth stack */
    ESP_ERROR_CHECK(ble_adv_init(DEVICE_NAME));

    /* 3. Wait for BLE controller synchronization */
    ESP_LOGI(TAG, "Waiting for BLE controller sync...");
    ret = ble_adv_wait_synced(portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BLE controller failed to sync: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "BLE controller synchronized successfully");

    /* 4. Launch dedicated sensor broadcaster task */
    BaseType_t task_ret = xTaskCreate(sensor_broadcast_task,
                                      "sensor_task",
                                      SENSOR_TASK_STACK_SIZE,
                                      NULL,
                                      SENSOR_TASK_PRIORITY,
                                      NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor broadcast task");
    }
}
