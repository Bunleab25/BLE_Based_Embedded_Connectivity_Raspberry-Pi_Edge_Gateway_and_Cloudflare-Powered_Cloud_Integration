#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize NimBLE host stack and register GAP callbacks
 *
 * @param[in] device_name Device name to register in GAP service
 * @return ESP_OK on success, or an error code
 */
esp_err_t ble_adv_init(const char *device_name);

/**
 * @brief Block until the NimBLE host stack has successfully synced with controller
 *
 * @param[in] timeout_ticks Maximum FreeRTOS ticks to wait
 * @return ESP_OK if synced, ESP_ERR_TIMEOUT if timed out
 */
esp_err_t ble_adv_wait_synced(TickType_t timeout_ticks);

/**
 * @brief Update and broadcast raw non-connectable BLE advertising data
 *
 * @param[in] data Advertising payload buffer
 * @param[in] len  Length of advertising payload (max 31 bytes)
 * @return ESP_OK on success, or error code
 */
esp_err_t ble_adv_update_payload(const uint8_t *data, uint8_t len);

/**
 * @brief Stop active BLE advertising
 *
 * @return ESP_OK on success, or error code
 */
esp_err_t ble_adv_stop(void);

#ifdef __cplusplus
}
#endif
