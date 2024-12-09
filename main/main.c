/* ESP32 WiFi Scanning example */

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi_types.h"

#define MAX_NETWORKS 20
#define MAX_SSID_LEN 33
#define TAG "WiFiScan"

typedef struct {
    uint8_t ssid[MAX_SSID_LEN];
    int32_t rssi;
    wifi_auth_mode_t auth_mode;
} wifi_network_info_t;

// Static allocation for large arrays
static wifi_network_info_t saved_networks[MAX_NETWORKS];
static wifi_ap_record_t ap_records[MAX_NETWORKS];
static int32_t saved_network_count = 0;  // Changed to int32_t to match nvs type

void save_networks_to_nvs() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return;

    nvs_set_blob(nvs_handle, "networks", saved_networks, sizeof(wifi_network_info_t) * saved_network_count);
    nvs_set_i32(nvs_handle, "net_count", saved_network_count);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
}

void load_networks_from_nvs() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return;

    size_t required_size = sizeof(wifi_network_info_t) * MAX_NETWORKS;
    nvs_get_i32(nvs_handle, "net_count", &saved_network_count);
    nvs_get_blob(nvs_handle, "networks", saved_networks, &required_size);
    nvs_close(nvs_handle);
}

// Add this function to check if network already exists
static bool network_exists(const uint8_t *ssid) {
    for (int i = 0; i < saved_network_count; i++) {
        if (strcmp((char *)saved_networks[i].ssid, (char *)ssid) == 0) {
            return true;
        }
    }
    return false;
}

static void wifi_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_scan_task(void *pvParameters) {
    while (1) {
        ESP_LOGI(TAG, "Scanning...");
        
        wifi_scan_config_t scan_config = {
            .ssid = 0,
            .bssid = 0,
            .channel = 0,
            .show_hidden = true
        };
        
        ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
        
        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);
        
        if (ap_count == 0) {
            ESP_LOGI(TAG, "No networks found.");
        } else {
            uint16_t max_aps = MAX_NETWORKS;
            esp_err_t scan_result = esp_wifi_scan_get_ap_records(&max_aps, ap_records);
            
            if (scan_result == ESP_OK) {
                bool networks_added = false;
                
                // Process each found network
                for (int i = 0; i < ap_count && saved_network_count < MAX_NETWORKS; i++) {
                    if (!network_exists(ap_records[i].ssid)) {
                        // Add new network to saved list
                        memcpy(saved_networks[saved_network_count].ssid, 
                               ap_records[i].ssid, MAX_SSID_LEN);
                        saved_networks[saved_network_count].rssi = ap_records[i].rssi;
                        saved_networks[saved_network_count].auth_mode = ap_records[i].authmode;
                        
                        ESP_LOGI(TAG, "New network found - %ld: %s (%ld) %s", 
                            (long)(saved_network_count + 1), 
                            (char *)saved_networks[saved_network_count].ssid,
                            saved_networks[saved_network_count].rssi,
                            saved_networks[saved_network_count].auth_mode == WIFI_AUTH_OPEN ? " " : "*"
                        );
                        
                        saved_network_count++;
                        networks_added = true;
                    }
                }
                
                // Only save to NVS if new networks were added
                if (networks_added) {
                    save_networks_to_nvs();
                    ESP_LOGI(TAG, "Updated network list saved to memory");
                } else {
                    ESP_LOGI(TAG, "No new networks found");
                }
            } else {
                ESP_LOGE(TAG, "Failed to get scan results");
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init();
    load_networks_from_nvs();

    ESP_LOGI(TAG, "Setup done!");
    if (saved_network_count > 0) {
        ESP_LOGI(TAG, "Loaded %ld networks from memory", saved_network_count);
        for (int i = 0; i < saved_network_count; i++) {
            ESP_LOGI(TAG, "%ld: %s (%ld) %s", 
                (long)(i + 1), 
                (char *)saved_networks[i].ssid,  // Cast to char* for printing
                saved_networks[i].rssi,
                saved_networks[i].auth_mode == WIFI_AUTH_OPEN ? " " : "*"
            );
        }
    }

    // Create WiFi scan task with larger stack
    xTaskCreate(wifi_scan_task, "wifi_scan", 4096, NULL, 5, NULL);
}