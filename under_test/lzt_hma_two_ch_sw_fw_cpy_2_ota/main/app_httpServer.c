#include "app_main.h"
#include <string.h>
#include <esp_http_server.h>
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
//#include "esp_efuse.h"

//#define ESP_MESH_HUB_ID "24:1a:c4:af:4c:fc"

#define MIN(a, b) a > b ? b : a

static const char *TAG = "http server";
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
    app_saveConfig();
    vTaskDelay(1000/portTICK_PERIOD_MS);
    esp_restart();
}
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
    .uri = "/update",
    .method = HTTP_GET,
    .handler = httpServer_setupHtmlGetHandler,
    .user_ctx = NULL
};

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
    .uri = "/update",
    .method = HTTP_POST,
    .handler = httpServer_setupPostHandler,
    .user_ctx = NULL
};

static esp_err_t httpServer_infoGettUriHandler(httpd_req_t *req){
    ESP_LOGI(TAG, "GET DEVICE JSON REQUEST");
    uint8_t derived_mac_addr[6] = {0};
    char macID[18];
    char nodeAddressJson[200];
    ESP_ERROR_CHECK(esp_read_mac(derived_mac_addr, ESP_MAC_WIFI_STA));
    sprintf(macID, "%x:%x:%x:%x:%x:%x", derived_mac_addr[0], derived_mac_addr[1], derived_mac_addr[2],
                                        derived_mac_addr[3], derived_mac_addr[4], derived_mac_addr[5]);
    
    sprintf(nodeAddressJson,"{\"id\": \"%s\", \"device_type\": \"2 Channel Switch\"}",macID);
    printf("\r\nDevice Address Json : %s\r\n",nodeAddressJson);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req,"Access-Control-Allow-Origin","*");
    httpd_resp_send(req, nodeAddressJson, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
static const httpd_uri_t httpServer_infoGettUri = {
    .uri = "/device_demo/device_info/",
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
    memset(storeBlock.appConfig.wifiSsid,'\0',sizeof(storeBlock.appConfig.wifiSsid));
    memset(storeBlock.appConfig.wifiPassword,'\0',sizeof(storeBlock.appConfig.wifiPassword));
    char* ssid = strstr(buf,"ssid");
    char* password =  strstr(buf,"password");
    char* seperator = strstr(buf,",");
    if(ssid != NULL && password != NULL && seperator != NULL){
        int ssidLength=strlen(ssid+6)-strlen(seperator);
        strncpy(storeBlock.appConfig.wifiSsid,(ssid+7),(ssidLength-2));
        int passwordLength=strlen(password+10);
        strncpy(storeBlock.appConfig.wifiPassword,(password+11),(passwordLength-3));
        printf("SSID: %s\r\n",storeBlock.appConfig.wifiSsid);
        printf("PASSWORD: %s\r\n",storeBlock.appConfig.wifiPassword);
        storeBlock.appConfig.startMesh= true;
        app_saveConfig();
        nodeAddressJson="{\"success\": \"1\", \"message\": \"WIiFi Credentials Receive Successful!\"}";
        printf("\r\nDevice Address Json : %s\r\n",nodeAddressJson);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req,"Access-Control-Allow-Origin","*");
        httpd_resp_send(req, nodeAddressJson, HTTPD_RESP_USE_STRLEN);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart(); //Restrating CPU on Successful Credential Receive
    }
    else{
        nodeAddressJson="{\"success\": \"0\", \"message\": \"WIiFi Credentials Received Failed!\"}";
        printf("\r\nDevice Address Json : %s\r\n",nodeAddressJson);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req,"Access-Control-Allow-Origin","*");
        httpd_resp_send(req, nodeAddressJson, HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}
static const httpd_uri_t httpServer_credentialPostUri = {
    .uri = "/device_demo/set_config/",
    .method = HTTP_POST,
    .handler = httpServer_credentialPostHandler,
    .user_ctx = NULL
};

static esp_err_t httpServer_optionsHandler(httpd_req_t *req)
{    
    printf("\r\nServed options request\r\n");
    httpd_resp_set_hdr(req,"Access-Control-Allow-Origin","*");
    httpd_resp_set_hdr(req,"Access-Control-Allow-Methods","*");
    httpd_resp_set_hdr(req,"Access-Control-Allow-Headers", "X-PINGOTHER, Content-Type");
    httpd_resp_set_hdr(req,"Access-Control-Max-Age", "86400");
    httpd_resp_send(req, NULL, 0);    
    return ESP_OK;
}
static const httpd_uri_t httpServer_putOptionsUri = {
    .uri = "/device_demo/set_config/",
    .method = HTTP_OPTIONS,
    .handler = httpServer_optionsHandler,
    .user_ctx = NULL
};
esp_err_t httpServer_errorHandler(httpd_req_t *req, httpd_err_code_t err)
{
    printf("\r\nHttp Error : %d\r\n", err);
    extern const unsigned char setup_html_start[] asm("_binary_setup_html_start");
    extern const unsigned char setup_html_end[] asm("_binary_setup_html_end");
    const size_t setup_html_size = (setup_html_end - setup_html_start);
    httpd_resp_set_status(req, "301 Moved Permanently");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/update");
    httpd_resp_send_chunk(req, (const char *)setup_html_start, setup_html_size);

    //terminate html header
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}
httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &httpServer_credentialPostUri);
        httpd_register_uri_handler(server, &httpServer_infoGettUri);
        httpd_register_uri_handler(server, &httpServer_putOptionsUri);
        //httpd_register_uri_handler(server, &httpServer_faviconGetUri);
        httpd_register_uri_handler(server, &httpServer_setupHtmlGetUri);
        httpd_register_uri_handler(server, &httpServer_setupPostUri);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, httpServer_errorHandler);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}