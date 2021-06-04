#include "app_main.h"
#include <string.h>
#include <esp_http_server.h>

//#define ESP_MESH_HUB_ID "24:1a:c4:af:4c:fc"

#define MIN(a, b) a > b ? b : a

static const char *TAG = "http server";

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
static const httpd_uri_t httpServer_optionsUri = {
    .uri = "/device_demo/set_config/",
    .method = HTTP_OPTIONS,
    .handler = httpServer_optionsHandler,
    .user_ctx = NULL
};
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
        httpd_register_uri_handler(server, &httpServer_optionsUri);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}