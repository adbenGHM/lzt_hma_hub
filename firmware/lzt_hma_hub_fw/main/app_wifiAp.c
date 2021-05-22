#include <string.h>
#include "esp_wifi.h"
#include <esp_http_server.h>
#include "cJSON.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "app_main.h"

#define APP_CONFIG_AP_WIFI_SSID         "esp32_ap"
#define APP_CONFIG_AP_WIFI_PASS         "esp@connect"
#define APP_CONFIG_AP_WIFI_CHANNEL      1
#define APP_CONFIG_AP_WIFI_MAX_STA_CONN 5
#define ESP_MESH_HUB_ID "24:1a:c4:af:4c:fc"

#define MIN(a, b) a > b ? b : a

static uint8_t freshScanInProgress = 0;
static const char *TAG = "wifi softAP";
// Purpose : Get device info from mobile
// URL : http://<static ip>:<port>/device_info
// Method : GET
// Response type : application/json
// Response body : {"id": <device id in string>, "device_type": <device type in string>}
// Response Code : 200


// Purpose : Set config to the device from mobile
// URL : http://<static ip>:<port>/set_config
// Method : POST
// Request type : application/json
// Request body : {"ssid": <wifi ssid in string>, "password": <wifi password in string>}
// Response type : application/json
// Response body : {"success": <1 or 0>, "message": <success / error message in string>}
// Response Code : 200

static esp_err_t httpServer_infoGettUriHandler(httpd_req_t *req){
    ESP_LOGI(TAG, "GET DEVICE JSON REQUEST");
    char nodeAddressJson[]="{\"id\": \""ESP_MESH_HUB_ID"\", \"device_type\": \"Single Channel Switch\"}";
    printf("\r\nDevice Address Json : %s\r\n",nodeAddressJson);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, nodeAddressJson, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
static const httpd_uri_t httpServer_infoGettUri = {
    .uri = "/device_info",
    .method = HTTP_GET,
    .handler = httpServer_infoGettUriHandler,
    .user_ctx = NULL};
static esp_err_t httpServer_credentialPostHandler(httpd_req_t *req)
{
    char buf[200];
    memset(buf,'\0',sizeof(buf));
    int ret, remaining = req->content_len;
    char* nodeAddressJson;
    while (remaining > 0)
    {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                                  MIN(remaining, sizeof(buf)))) <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                /* Retry receiving if timeout occurred */
                continue;
            }
        }
        remaining -= ret;
    }
    printf("\r\nHttp Client Post received : %s\r\n",buf);
    memset(appConfig.wifiSsid,'\0',sizeof(appConfig.wifiSsid));
    memset(appConfig.wifiPassword,'\0',sizeof(appConfig.wifiPassword));
    char* ssid = strstr(buf,"ssid");
    char* password =  strstr(buf,"password");
    char* seperator = strstr(buf,",");
    if(ssid != NULL && password != NULL && seperator != NULL){
        int ssidLength=strlen(ssid+6)-strlen(seperator);
        strncpy(appConfig.wifiSsid,(ssid+7),(ssidLength-2));
        int passwordLength=strlen(password+10);
        strncpy(appConfig.wifiPassword,(password+11),(passwordLength-3));
        printf("SSID: %s\r\n",appConfig.wifiSsid);
        printf("PASSWORD: %s\r\n",appConfig.wifiPassword);
        appConfig.startMesh= true;
        app_saveConfig();
        vTaskDelete(&ap_indicator_TaskHandle);
        control_Ind_Led(0);
        nodeAddressJson="{\"success\": 1, \"message\": WIiFi Credentials Receive Successful!}";
        printf("\r\nDevice Address Json : %s\r\n",nodeAddressJson);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, nodeAddressJson, HTTPD_RESP_USE_STRLEN);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart(); //Restrating CPU on Successful Credential Receive
    }
    else{
        nodeAddressJson="{\"success\": 0, \"message\": WIiFi Credentials Received Failed!}";
        printf("\r\nDevice Address Json : %s\r\n",nodeAddressJson);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, nodeAddressJson, HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}
static const httpd_uri_t httpServer_credentialPostUri = {
    .uri = "/set_config",
    .method = HTTP_POST,
    .handler = httpServer_credentialPostHandler,
    .user_ctx = NULL
};
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        //httpd_register_uri_handler(server, &httpServer_faviconGetUri);
        //httpd_register_uri_handler(server, &httpServer_getUri);
        // httpd_register_uri_handler(server, &httpServer_jqueryGetUri);
        // httpd_register_uri_handler(server, &httpServer_setupCssGetUri);
        // httpd_register_uri_handler(server, &httpServer_setupJsGetUri);
        // httpd_register_uri_handler(server, &httpServer_setupHtmlGetUri);
        httpd_register_uri_handler(server, &httpServer_credentialPostUri);
        httpd_register_uri_handler(server, &httpServer_infoGettUri);
        // httpd_register_uri_handler(server, &httpServer_scannedAccessPointsJsonGetUri);
        // httpd_register_uri_handler(server, &httpServer_deviceListJsonGetUri);
        // httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, httpServer_errorHandler);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}
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