#include <stdio.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "host/util/util.h" // Add this at the top

static const char *TAG = "XBOX_SCAN";
static uint16_t conn_handle; // Stores the active connection ID

void ble_app_on_sync(void);

long hex_to_int(const char *hex_str) {
    char *endptr;
    // The '16' tells the function to interpret the string as Hexadecimal
    long value = strtol(hex_str, &endptr, 16);

    // Check if the string was actually a valid hex number
    if (hex_str == endptr) {
        printf("Error: No valid hex digits found.\n");
        return 0;
    }

    return value;
}

// This runs when the connection status changes
static int ble_gap_connect_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "CONNECTED TO XBOX! Initiating security...");
                conn_handle = event->connect.conn_handle;
                
                // TRIGGER PAIRING IMMEDIATELY
                int rc = ble_gap_security_initiate(conn_handle);
                if (rc != 0) {
                    ESP_LOGE(TAG, "Security initiation failed: %d", rc);
                }
            } else {
                ESP_LOGE(TAG, "Connection failed; status=%d", event->connect.status);
                // Restart scanning if connection failed
                ble_app_on_sync(); 
            }
            return 0;

        case BLE_GAP_EVENT_ENC_CHANGE:
            if (event->enc_change.status == 0) {
                ESP_LOGI(TAG, "ENCRYPTION CHANGED: PAIRING SUCCESSFUL!");
                // THIS is when the Xbox light should go solid
            } else {
                ESP_LOGE(TAG, "Encryption failed; status=%d", event->enc_change.status);
            }
            return 0;
        case BLE_GAP_EVENT_NOTIFY_RX:
            // This is where the raw joystick data arrives!
            int Lx = event->notify_rx.om->om_data[1]-128;
            int Ly = event->notify_rx.om->om_data[3]-128;
            int Rx = event->notify_rx.om->om_data[5]-128;
            int Ry = event->notify_rx.om->om_data[7]-128;
            unsigned int Lt = event->notify_rx.om->om_data[8]*event->notify_rx.om->om_data[9];
            unsigned int Rt = event->notify_rx.om->om_data[10]*event->notify_rx.om->om_data[11];
            printf("Received Data: ");
            //for (int i = 0; i < event->notify_rx.om->om_len; i++) {
            //    printf("%02x ", event->notify_rx.om->om_data[i]);
            //}
            printf("Left X: %d, Left Y: %d, Right X: %d, Right Y: %d, Left Trigger: %u, Right Trigger %u", Lx, Ly, Rx, Ry, Lt, Rt);
            printf("\n");
            return 0;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "DISCONNECTED. Reason: %d. Resuming scan...", event->disconnect.reason);
            ble_app_on_sync();
            return 0;
    }
    return 0;
}

static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_hs_adv_fields fields;
    if (event->type == BLE_GAP_EVENT_DISC) {
        ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
        
        if (fields.name_len > 0 && strncmp((char *)fields.name, "Xbox Wireless Controller", fields.name_len) == 0) {
            ESP_LOGI(TAG, "XBOX SPOTTED! Attempting to connect...");
            
            // STOP scanning so we can connect
            ble_gap_disc_cancel();

            // CONNECT to the address we just found
            ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &event->disc.addr, 30000, NULL, ble_gap_connect_event, NULL);
        }
    }
    return 0;
}

void ble_app_on_sync(void) {
    printf("NimBLE Host Synced. Starting Scan...\n");
    struct ble_gap_disc_params disc_params = {0};
    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
}

void host_task(void *param) {
    nimble_port_run();
}

void app_main(void) {
    // 1. Initialize NVS (CRITICAL for Bluetooth)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    nimble_port_init();

    // SET SECURITY OPTIONS
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO; // Xbox doesn't have a screen/keyboard
    ble_hs_cfg.sm_bonding = 1;                 // Store the keys
    ble_hs_cfg.sm_mitm = 1;                    // Man-in-the-middle protection
    ble_hs_cfg.sm_sc = 1;                      // Secure Connections (Level 4)
    
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    nimble_port_freertos_init(host_task);
}