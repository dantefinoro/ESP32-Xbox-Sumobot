#include <stdio.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "host/util/util.h" 
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "motor_control.h"
/* if session is restarted enter
. $HOME/esp/esp-idf/export.sh
cd ~/Desktop/sumo/sumobot_v1
idf.py --version
idf.py flash monitor
 */


static const char *TAG = "XBOX_SCAN";
static uint16_t conn_handle; // Stores the active connection ID


//init functions
void ble_app_on_sync(void);
long hex_to_int(const char *hex_str);
static int ble_gap_connect_event(struct ble_gap_event *event, void *arg);
static int ble_gap_event(struct ble_gap_event *event, void *arg);

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
            // raw joystick data
            int Lx = event->notify_rx.om->om_data[1]-128;
            int Ly = event->notify_rx.om->om_data[3]-128;
            int Rx = event->notify_rx.om->om_data[5]-128;
            int Ry = event->notify_rx.om->om_data[7]-128;
            unsigned int Lt = event->notify_rx.om->om_data[8]*event->notify_rx.om->om_data[9]/3;
            unsigned int Rt = event->notify_rx.om->om_data[10]*event->notify_rx.om->om_data[11]/3;
            handle_controller(Lx, Lt, Rt);
            //printf("Received Data: ");
            //for (int i = 0; i < event->notify_rx.om->om_len; i++) {
            //    printf("%02x ", event->notify_rx.om->om_data[i]);
            //}
            //printf("Left X: %d, Left Y: %d, Right X: %d, Right Y: %d, Left Trigger: %u, Right Trigger %u", Lx, Ly, Rx, Ry, Lt, Rt);
            //printf("\n");
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
            
            // 1. STOP scanning
            ble_gap_disc_cancel();

            // 2. Define the "Patience" of the connection
            struct ble_gap_conn_params params = {
                .scan_itvl = 16,
                .scan_window = 16,
                .itvl_min = 24,               // 30ms connection interval
                .itvl_max = 40,               // 50ms connection interval
                .latency = 0,
                .supervision_timeout = 512,     // 5.12 seconds (Crucial for stability)
                .min_ce_len = 0,
                .max_ce_len = 0,
            };

            // 3. CONNECT using the params
            int rc = ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &event->disc.addr, 30000, &params, ble_gap_connect_event, NULL);
            
            if (rc != 0) {
                ESP_LOGE(TAG, "Error: Failed to initiate connection; rc=%d", rc);
            }
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
    init_motors();
    // 1. Initialize NVS
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