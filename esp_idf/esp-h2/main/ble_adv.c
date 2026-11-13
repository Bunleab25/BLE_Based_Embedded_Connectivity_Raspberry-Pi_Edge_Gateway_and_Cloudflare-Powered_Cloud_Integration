#include "ble_adv.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

static const char *TAG = "ble_adv";

#define BLE_SYNC_BIT BIT0

static EventGroupHandle_t s_ble_event_group;
static uint8_t s_own_addr_type;

static void on_ble_sync(void)
{
    int rc = ble_hs_id_infer_auto(0, &s_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to infer address type: %d", rc);
        return;
    }

    uint8_t addr_val[6] = {0};
    ble_hs_id_copy_addr(s_own_addr_type, addr_val, NULL);
    ESP_LOGI(TAG, "NimBLE host synced. Device Address: %02x:%02x:%02x:%02x:%02x:%02x",
             addr_val[5], addr_val[4], addr_val[3], addr_val[2], addr_val[1], addr_val[0]);

    if (s_ble_event_group != NULL) {
        xEventGroupSetBits(s_ble_event_group, BLE_SYNC_BIT);
    }
}

static void on_ble_reset(int reason)
{
    ESP_LOGW(TAG, "NimBLE stack reset, reason: %d", reason);
    if (s_ble_event_group != NULL) {
        xEventGroupClearBits(s_ble_event_group, BLE_SYNC_BIT);
    }
}

static void nimble_host_task(void *param)
{
    ESP_LOGI(TAG, "NimBLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

esp_err_t ble_adv_init(const char *device_name)
{
    s_ble_event_group = xEventGroupCreate();
    if (s_ble_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create FreeRTOS event group");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init failed: %s", esp_err_to_name(err));
        return err;
    }

    ble_hs_cfg.sync_cb = on_ble_sync;
    ble_hs_cfg.reset_cb = on_ble_reset;

    if (device_name != NULL && strlen(device_name) > 0) {
        ble_svc_gap_device_name_set(device_name);
    }

    nimble_port_freertos_init(nimble_host_task);
    return ESP_OK;
}

esp_err_t ble_adv_wait_synced(TickType_t timeout_ticks)
{
    if (s_ble_event_group == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    EventBits_t bits = xEventGroupWaitBits(s_ble_event_group,
                                           BLE_SYNC_BIT,
                                           pdFALSE,
                                           pdTRUE,
                                           timeout_ticks);

    return (bits & BLE_SYNC_BIT) ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t ble_adv_update_payload(const uint8_t *data, uint8_t len)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Stop active advertising if currently running */
    if (ble_gap_adv_active()) {
        int rc = ble_gap_adv_stop();
        if (rc != 0 && rc != BLE_HS_EALREADY) {
            ESP_LOGW(TAG, "ble_gap_adv_stop warning: %d", rc);
        }
    }

    int rc = ble_gap_adv_set_data(data, len);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_set_data failed: %d", rc);
        return ESP_FAIL;
    }

    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_NON,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
        .itvl_min  = BLE_GAP_ADV_ITVL_MS(100),
        .itvl_max  = BLE_GAP_ADV_ITVL_MS(150),
    };

    rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_start failed: %d", rc);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t ble_adv_stop(void)
{
    if (ble_gap_adv_active()) {
        int rc = ble_gap_adv_stop();
        if (rc != 0 && rc != BLE_HS_EALREADY) {
            ESP_LOGE(TAG, "ble_gap_adv_stop failed: %d", rc);
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}
