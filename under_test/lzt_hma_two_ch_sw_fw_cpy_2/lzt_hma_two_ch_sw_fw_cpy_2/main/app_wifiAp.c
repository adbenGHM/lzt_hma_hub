#include <string.h>
#include "esp_event.h"
#include "nvs_flash.h"
#include "app_wifi.h"
#include "app_main.h"

// #define APP_CONFIG_AP_WIFI_SSID         "lazot"
// #define APP_CONFIG_AP_WIFI_PASS         "12345678"
#define APP_CONFIG_AP_WIFI_SSID         "esp32_ap"
#define APP_CONFIG_AP_WIFI_PASS         "esp@connect"
#define APP_CONFIG_AP_WIFI_CHANNEL      1
#define APP_CONFIG_AP_WIFI_MAX_STA_CONN 5

static uint8_t freshScanInProgress = 0;
static const char *TAG = "wifi softAP";


static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_START)
    {
        ESP_LOGI(TAG, "AP Started");
        if (!freshScanInProgress)
        {
            ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, false));
            freshScanInProgress = 1;
        }
        start_webserver();
    }
    else if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}
app_status_t app_wifiApInit()
{
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                               ESP_EVENT_ANY_ID,
                                               &wifi_event_handler,
                                               NULL));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = APP_CONFIG_AP_WIFI_SSID,
            .ssid_len = strlen(APP_CONFIG_AP_WIFI_SSID),
            .channel = APP_CONFIG_AP_WIFI_CHANNEL,
            .password = APP_CONFIG_AP_WIFI_PASS,
            .max_connection = APP_CONFIG_AP_WIFI_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    if (strlen(APP_CONFIG_AP_WIFI_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             APP_CONFIG_AP_WIFI_SSID, APP_CONFIG_AP_WIFI_PASS, APP_CONFIG_AP_WIFI_CHANNEL);
    return APP_STATUS_OK;
}