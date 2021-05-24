#ifndef __APP_MAIN__
#define __APP_MAIN__

#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sys.h"

//==============================================================================================
#define BUTTON_IN1              GPIO_NUM_1
#define BUTTON_IN2              GPIO_NUM_2
#define RELAY_OUT1              GPIO_NUM_12
#define RELAY_OUT2              GPIO_NUM_13
#define RELAY1_ON_IND_LED       GPIO_NUM_5
#define RELAY1_OFF_IND_LED      GPIO_NUM_4
#define RELAY2_ON_IND_LED       GPIO_NUM_7
#define RELAY2_OFF_IND_LED      GPIO_NUM_6
#define AUTO_CUTOFF_MILLIS 10000
//==============================================================================================
#define APP_CONFIG_NODE_ID_LEN 20 //size of node ID in Bytes

#define APP_CONFIG_NODE_CMD_QUEUE_SIZE 10 //maximum no of node commads that could be queued
#define APP_CONFIG_NODE_RESPONSE_QUEUE_SIZE 10 //maximum no fo repose from node that can be queued 

#define APP_CONFIG_MQTT_BROKER_URL "mqtt://test.mosquitto.org"
#define APP_CONFIG_MQTT_BROKER_PORT "1883"

//==============================MUST BE SAME FOR BOTH NODE AND HUB==============================
#define APP_CONFIG_NODE_DATA_MAX_LEN 200 //maximum size of node data in bytes

#define APP_CONFIG_MESH_AP_PASSWD "Arghya@123"
// #define APP_CONFIG_MESH_ROUTER_SSID "Soumyanetra"
// #define APP_CONFIG_MESH_ROUTER_PASSWD "Twamoshi"

static const uint8_t APP_CONFIG_MESH_ID[6] = {0x77, 0x77, 0x77, 0x77, 0x77, 0x77};
//===============================================================================================

//do not change
#define APP_MAX_MQTT_DATA_LENGTH (APP_CONFIG_NODE_DATA_MAX_LEN+APP_CONFIG_NODE_ID_LEN+100)

typedef enum
{
    APP_STATUS_ERROR_NVS=-3,
    APP_STATUS_ERROR_STA_MODE_CONNECTION_FAILED=-2,
    APP_STATUS_ERROR = -1,
    APP_STATUS_OK = 0,
} app_status_t;
typedef struct{
    char nodeId[APP_CONFIG_NODE_ID_LEN+1]; //mac ID of the target Node
    char data[APP_CONFIG_NODE_DATA_MAX_LEN+1];
}app_nodeData_t;

struct appConf
{
    char wifiSsid[32];     //should not be changed
    char wifiPassword[64]; //should not be changed
    bool startMesh;
} appConfig;

typedef struct{
    gpio_num_t buttonGpioNum;
    uint8_t pressCount;
} button_details_t;

/*
*@brief Queus the commands, of type app_nodeData_t,
*       targeted to a Node. The maximum no of commands
        in the queue is limited by "APP_CONFIG_NODE_CMD_QUEUE_SIZE"
*/
QueueHandle_t app_nodeCommandQueue; 
QueueHandle_t app_nodeResponseQueue;
QueueHandle_t app_buttonDetailsQueue;

uint64_t app_millis;

app_status_t app_meshHubInit(void);
app_status_t app_consolInit(void);
app_status_t app_consolRegisterNodeCmd(void);
app_status_t app_mqttClientInit();
app_status_t app_wifiApInit(void);
app_status_t app_saveConfig(void);
app_status_t app_loadConfig(void);
void app_userInputInit();
//void app_meshInit(void);
app_status_t app_wifiStaInit(void);
void app_process_button_input_Task(void*);
void app_process_cmd_input_Task(void* pvParameters);

void initialise_timer(void);
void control_Ind_Led(uint32_t);

uint64_t getMilis(void);


#endif