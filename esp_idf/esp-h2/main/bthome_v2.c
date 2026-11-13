#include "bthome_v2.h"
#include <string.h>

/* BLE Advertising Data Type Constants */
#define BLE_GAP_AD_TYPE_FLAGS         (0x01)
#define BLE_GAP_AD_TYPE_NAME_COMPLETE (0x09)
#define BLE_GAP_AD_TYPE_SERVICE_DATA  (0x16)

/* BLE Advertising Flags */
#define BLE_GAP_FLAGS_LE_GEN_DISC     (0x02)
#define BLE_GAP_FLAGS_BR_EDR_NOT_SUPP (0x04)

esp_err_t bthome_v2_build_adv_packet(const char *device_name,
                                     const bthome_sensor_data_t *data,
                                     uint8_t *buffer,
                                     size_t max_len,
                                     size_t *out_len)
{
    if (data == NULL || buffer == NULL || out_len == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t offset = 0;

    /* 1. Flags AD Structure: Length (2), Type (0x01), Value (0x06) */
    if (offset + 3 > max_len) {
        return ESP_ERR_NO_MEM;
    }
    buffer[offset++] = 0x02;
    buffer[offset++] = BLE_GAP_AD_TYPE_FLAGS;
    buffer[offset++] = BLE_GAP_FLAGS_LE_GEN_DISC | BLE_GAP_FLAGS_BR_EDR_NOT_SUPP;

    /* 2. Complete Local Name AD Structure (optional) */
    if (device_name != NULL && strlen(device_name) > 0) {
        size_t name_len = strlen(device_name);
        if (offset + 2 + name_len > max_len) {
            return ESP_ERR_NO_MEM;
        }
        buffer[offset++] = (uint8_t)(name_len + 1);
        buffer[offset++] = BLE_GAP_AD_TYPE_NAME_COMPLETE;
        memcpy(&buffer[offset], device_name, name_len);
        offset += name_len;
    }

    /* 3. BTHome v2 Service Data (UUID 0xFCD2) */
    /* Service Data Content:
     * - UUID16 (2 bytes: 0xD2, 0xFC)
     * - Header (1 byte: 0x40)
     * - Packet ID Object: ID (1 byte: 0x00) + Value (1 byte)
     * - Temperature Object: ID (1 byte: 0x02) + Value (2 bytes, Little Endian)
     * - Humidity Object: ID (1 byte: 0x03) + Value (2 bytes, Little Endian)
     * Total service data payload = 2 + 1 + 2 + 3 + 3 = 11 bytes + 1 type byte = 12 bytes length
     */
    const uint8_t svc_data_len = 1 + 2 + 1 + 2 + 3 + 3; /* Type byte + 11 content bytes = 12 */

    if (offset + 1 + svc_data_len > max_len) {
        return ESP_ERR_NO_MEM;
    }

    buffer[offset++] = svc_data_len;
    buffer[offset++] = BLE_GAP_AD_TYPE_SERVICE_DATA;

    /* 16-bit UUID (Little Endian: 0xD2, 0xFC) */
    buffer[offset++] = (uint8_t)(BTHOME_UUID16 & 0xFF);
    buffer[offset++] = (uint8_t)((BTHOME_UUID16 >> 8) & 0xFF);

    /* BTHome v2 Header */
    buffer[offset++] = BTHOME_V2_HEADER_UNENCRYPTED;

    /* Packet ID */
    buffer[offset++] = BTHOME_OBJ_PACKET_ID;
    buffer[offset++] = data->packet_id;

    /* Temperature (signed 16-bit, 0.01 °C, Little Endian) */
    buffer[offset++] = BTHOME_OBJ_TEMPERATURE;
    buffer[offset++] = (uint8_t)(data->temperature_centi & 0xFF);
    buffer[offset++] = (uint8_t)((data->temperature_centi >> 8) & 0xFF);

    /* Humidity (unsigned 16-bit, 0.01 %, Little Endian) */
    buffer[offset++] = BTHOME_OBJ_HUMIDITY;
    buffer[offset++] = (uint8_t)(data->humidity_centi & 0xFF);
    buffer[offset++] = (uint8_t)((data->humidity_centi >> 8) & 0xFF);

    *out_len = offset;
    return ESP_OK;
}
