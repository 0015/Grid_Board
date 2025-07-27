#include "ble_server.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "bleprph.h"

static void (*conn_cb)(bool) = NULL;
static void (*write_cb)(const uint8_t *, uint16_t) = NULL;

static int ble_gap_event_cb(struct ble_gap_event *event, void *arg);

static void ble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void ble_on_sync(void)
{
    int rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    uint8_t addr_val[6];
    ble_hs_id_copy_addr(BLE_OWN_ADDR_PUBLIC, addr_val, NULL);
    ESP_LOGI("BLE_SERVER", "Device Address: %02X:%02X:%02X:%02X:%02X:%02X",
             addr_val[5], addr_val[4], addr_val[3], addr_val[2], addr_val[1], addr_val[0]);

    struct ble_gap_adv_params adv_params = {0};
    struct ble_hs_adv_fields fields = {0};

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    const char *name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    ble_gap_adv_set_fields(&fields);

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event_cb, NULL);
}

static void ble_on_reset(int reason)
{
    ESP_LOGE("BLE_SERVER", "Resetting state; reason=%d", reason);
}

static int ble_gap_event_cb(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        if (conn_cb)
            conn_cb(event->connect.status == 0);
        if (event->connect.status != 0)
            ble_on_sync();
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        if (conn_cb)
            conn_cb(false);
        ble_on_sync();
        break;
    default:
        break;
    }
    return 0;
}

void ble_server_register_callbacks(void (*on_connect)(bool), void (*on_write)(const uint8_t *, uint16_t))
{
    conn_cb = on_connect;
    write_cb = on_write;
    gatt_svr_set_write_callback(write_cb);
}

void ble_server_start(const char *device_name)
{
    nimble_port_init();
    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    gatt_svr_init();
    ble_svc_gap_init();
    ble_svc_gatt_init();

    if (device_name)
    {
        ble_svc_gap_device_name_set(device_name);
    }

    nimble_port_freertos_init(ble_host_task);
}