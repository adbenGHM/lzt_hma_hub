
#include "app_main.h"
#include "task.h"
//#include "cJSON.h"
static const char *TAG = "wifi softAP";
//static char accessPointJson[3000];
static uint8_t freshScanInProgress = 0;
#define BUFFSIZE_OTA 10240
/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE_OTA+1] = { 0 };

static esp_ota_handle_t update_handle = 0 ;
static const esp_partition_t *update_partition = NULL;
static esp_err_t err;


static void __attribute__((noreturn)) task_fatal_error()
{
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    (void)vTaskDelete(NULL);

    while (1) {
        ;
    }
}
void ota_example_init()
{
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */

    ESP_LOGI(TAG, "Starting OTA example...");


    update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        //http_cleanup(client);
        task_fatal_error();
    }
    ESP_LOGI(TAG, "esp_ota_begin succeeded");
}
void ota_end_task(){
    if (esp_ota_end(update_handle) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed!");
        //http_cleanup(client);
        task_fatal_error();
    }
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        //http_cleanup(client);
        task_fatal_error();
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
}

static esp_err_t httpServer_faviconGetHandler(httpd_req_t *req)
{
    extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
    extern const unsigned char favicon_ico_end[] asm("_binary_favicon_ico_end");
    ESP_LOGI(TAG, "Favicon Request");
    const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_size);
    return ESP_OK;
}
static const httpd_uri_t httpServer_faviconGetUri = {
    .uri = "/favicon.ico",
    .method = HTTP_GET,
    .handler = httpServer_faviconGetHandler,
    .user_ctx = NULL};
static esp_err_t httpServer_setupHtmlGetHandler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET SETUP HTML REQUEST");
    extern const unsigned char setup_html_start[] asm("_binary_setup_html_start");
    extern const unsigned char setup_html_end[] asm("_binary_setup_html_end");
    const size_t setup_html_size = (setup_html_end - setup_html_start);
    httpd_resp_send_chunk(req, (const char *)setup_html_start, setup_html_size);

    //terminate html header
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static const httpd_uri_t httpServer_setupHtmlGetUri = {
    .uri = "/setup",
    .method = HTTP_GET,
    .handler = httpServer_setupHtmlGetHandler,
    .user_ctx = NULL};

void format_buf(char* buf)
{
    char *ptr=strstr(buf,"task=");
    int taskLen=strlen(ptr);
    strncpy(buf,ptr+5,taskLen);
}
//------------------------------------------------------------------------------------
//POST HANDLER START
//------------------------------------------------------------------------------------

static esp_err_t httpServer_setupPostHandler(httpd_req_t *req)
{
    uint64_t availableConetctSize = req->content_len;
    int data_read;
    ota_write_details_t ota;
    uint8_t ota_flag=0;
    unsigned int ota_offset = 0;
    unsigned int ota_data_len = 0;
    printf("Available size: %llu\r\n",availableConetctSize);
    while (availableConetctSize>0) {
        
        if ((data_read = httpd_req_recv(req, ota_write_data, BUFFSIZE_OTA)) <= 0)
        {
            if (data_read == HTTPD_SOCK_ERR_TIMEOUT)
            {
                /* Retry receiving if timeout occurred */
                continue;
            }
            
        }
        else{
            ota_write_data[data_read] = '\0';
            availableConetctSize -= data_read;
            ota_offset = 0;
            for (int i = 0; i < data_read && !ota_flag; i++ ){
                if(ota_write_data[i]==0xE9){
                    ota_flag = 1;
                    ota_offset = i;
                }
            }
            if(!ota_flag)
                continue;
            ota.data = ota_write_data+ota_offset;
            ota.length = data_read-ota_offset;
            ota_data_len += ota.length ;
            
            err = esp_ota_write( update_handle, (const void *)ota.data, ota.length );
            if (err != ESP_OK) {
                task_fatal_error();
            }
        }
        
    }
   
    // 
    extern const unsigned char setup_html_start[] asm("_binary_setup_html_start");
    extern const unsigned char setup_html_end[] asm("_binary_setup_html_end");
    const size_t setup_html_size = (setup_html_end - setup_html_start);
    httpd_resp_send_chunk(req, (const char *)setup_html_start, setup_html_size);

    //terminate html header
    httpd_resp_sendstr_chunk(req, NULL);
    printf("\r\nOTA data length: %d, %x\r\n",ota_data_len,ota.data[ota.length-1]);
    ota_end_task();
    return ESP_OK;
}

static const httpd_uri_t httpServer_setupPostUri = {
    .uri = "/setup",
    .method = HTTP_POST,
    .handler = httpServer_setupPostHandler,
    .user_ctx = NULL};
//------------------------------------------------------------------------------------
//POST HANDLER END
//------------------------------------------------------------------------------------

esp_err_t httpServer_errorHandler(httpd_req_t *req, httpd_err_code_t err)
{
    printf("\r\nHttp Error : %d\r\n", err);
    extern const unsigned char setup_html_start[] asm("_binary_setup_html_start");
    extern const unsigned char setup_html_end[] asm("_binary_setup_html_end");
    const size_t setup_html_size = (setup_html_end - setup_html_start);
    httpd_resp_set_status(req, "301 Moved Permanently");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/setup");
    httpd_resp_send_chunk(req, (const char *)setup_html_start, setup_html_size);

    //terminate html header
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}
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
        httpd_register_uri_handler(server, &httpServer_faviconGetUri);
        //httpd_register_uri_handler(server, &httpServer_getUri);
        //httpd_register_uri_handler(server, &httpServer_jqueryGetUri);
        //httpd_register_uri_handler(server, &httpServer_setupCssGetUri);
        //httpd_register_uri_handler(server, &httpServer_setupJsGetUri);
        httpd_register_uri_handler(server, &httpServer_setupHtmlGetUri);
        httpd_register_uri_handler(server, &httpServer_setupPostUri);
        //httpd_register_uri_handler(server, &httpServer_scannedAccessPointsJsonGetUri);
        //httpd_register_uri_handler(server, &httpServer_deviceListJsonGetUri);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, httpServer_errorHandler);
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

    //esp_netif_t *wifiAP =
    esp_netif_create_default_wifi_ap();

    //chaging the ip of the access point
    // esp_netif_ip_info_t ipInfo;
    // uint8_t apIpAddress[]=APP_CONFIG_AP_IP_ADDRESS;
    // uint8_t apGatewayIpAddress[]=APP_CONG_AP_GATEWAY_IP_ADDRESS;
    // uint8_t apNetMask[]=APP_CONFIG_AP_NET_MASK;
    // IP4_ADDR(&ipInfo.ip, apIpAddress[0], apIpAddress[1], apIpAddress[2], apIpAddress[3]);
    // IP4_ADDR(&ipInfo.gw,apGatewayIpAddress[0], apGatewayIpAddress[1], apGatewayIpAddress[2], apGatewayIpAddress[3]);
    // IP4_ADDR(&ipInfo.netmask, apNetMask[0], apNetMask[1], apNetMask[2], apNetMask[3]);
    // esp_netif_dhcps_stop(wifiAP);
    // esp_netif_set_ip_info(wifiAP, &ipInfo);
    // esp_netif_dhcps_start(wifiAP);
    //----------------------------------------

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
    task_queue = xQueueCreate(queue_len, sizeof(int)); //queue creation
    return APP_STATUS_OK;
}