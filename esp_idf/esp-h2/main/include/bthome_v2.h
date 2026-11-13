#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BTHome v2 Specification Constants
 * Specification: https://bthome.io/format/
 */
#define BTHOME_UUID16                 (0xFCD2)
#define BTHOME_V2_HEADER_UNENCRYPTED  (0x40)

/* Object IDs */
#define BTHOME_OBJ_PACKET_ID          (0x00) /* uint8, 1 byte */
#define BTHOME_OBJ_TEMPERATURE        (0x02) /* int16 (factor 0.01 °C), 2 bytes */
#define BTHOME_OBJ_HUMIDITY           (0x03) /* uint16 (factor 0.01 %), 2 bytes */

/**
 * @brief Structure holding BTHome sensor measurements
 */
typedef struct {
    uint8_t  packet_id;
    int16_t  temperature_centi; /* Centidegrees Celsius (e.g. 2550 = 25.50 °C) */
    uint16_t humidity_centi;    /* Centipercent RH (e.g. 6050 = 60.50 %) */
} bthome_sensor_data_t;

/**
 * @brief Build a standard BLE advertisement packet with BTHome v2 payload
 *
 * @param[in]  device_name Complete local name to broadcast (can be NULL)
 * @param[in]  data        Pointer to sensor measurements
 * @param[out] buffer      Output buffer to write advertising payload
 * @param[in]  max_len     Maximum capacity of the output buffer (standard BLE: 31 bytes)
 * @param[out] out_len     Length of serialized packet written
 *
 * @return ESP_OK on success, ESP_ERR_NO_MEM if buffer is too small, ESP_ERR_INVALID_ARG on NULL data
 */
esp_err_t bthome_v2_build_adv_packet(const char *device_name,
                                     const bthome_sensor_data_t *data,
                                     uint8_t *buffer,
                                     size_t max_len,
                                     size_t *out_len);

#ifdef __cplusplus
}
#endif
