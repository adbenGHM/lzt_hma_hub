#ifndef __APP_MAIN_H__
#define __APP_MAIN_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_http_server.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"

//#include "esp_system.h"
#include "app_config.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_efuse.h"

#define MIN(a, b) a > b ? b : a
//#define APP_SOFT_RESET_GPIO 17
// typedef enum{
//     APP_ERROR_LOAD_CONFIG=0,
//     APP_ERROR_WIFI_STA_CONNECTION_FAILED=1,
//     APP_ERROR_TIME_SYNC=2,
//     APP_ERROR_FAILED_CREATING_NODE_DATA_QUEUE=3,
//     APP_ERROR_INITIALIZING_HTTP_CLIENT=4,
//     APP_ERROR_INITIALIZING_RFM69=5,
// }app_error_t;

typedef enum
{
    APP_STATUS_ERROR_NVS=-3,
    APP_STATUS_ERROR_STA_MODE_CONNECTION_FAILED=-2,
    APP_STATUS_ERROR = -1,
    APP_STATUS_OK = 0,
} app_status_t;

typedef struct{
    char* data;
    int length;
} ota_write_details_t;
// typedef enum{
//     APP_MODE_AP=0,
//     APP_MODE_STA=1
// }app_mode_t;

// typedef struct{
//     char address[APP_CONFIG_NODE_ADDRESS_LENGTH+1];
//     char data[APP_CONFIG_MAX_NODE_DATA_LEGTH+1];
//     struct tm timeOfRecieve;
// }app_nodeData_t;

QueueHandle_t nodeDataQueue;
QueueHandle_t otaQueue;

//app_mode_t appMode;

// app_status_t app_saveConfig();
// app_status_t app_loadConfig();
// app_status_t app_syncTime(void);
// uint8_t app_digitalInputFilter(int gpio, uint32_t filterPoints,uint32_t samplingDelayMillis);
// void app_nodeDataToJsonString(app_nodeData_t nodeData,char* p_JsonStr);

// app_status_t app_rfm69Init();

// app_status_t app_wifiStaInit(void);
app_status_t app_wifiApInit(void);

// app_status_t app_httpClientInit(void);

// void task_manager(char *);

void ota_example_init(void);

#endif
