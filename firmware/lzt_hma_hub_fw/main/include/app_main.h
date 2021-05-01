#ifndef __APP_MAIN__
#define __APP_MAIN__

#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"


#define APP_CONFIG_NODE_ID_LEN 20 //size of node ID in Bytes

#define APP_CONFIG_NODE_CMD_QUEUE_SIZE 10 //maximum no of node commads that could be queued
#define APP_CONFIG_NODE_RESPONSE_QUEUE_SIZE 10 //maximum no fo repose from node that can be queued 

#define APP_CONFIG_MQTT_BROKER_URL "mqtt://test.mosquitto.org"
#define APP_CONFIG_MQTT_BROKER_PORT "1883"

//==============================MUST BE SAME FOR BOTH NODE AND HUB==============================
#define APP_CONFIG_NODE_DATA_MAX_LEN 200 //maximum size of node data in bytes

#define APP_CONFIG_MESH_AP_PASSWD "Arghya@123"
#define APP_CONFIG_MESH_ROUTER_SSID "Typical"
#define APP_CONFIG_MESH_ROUTER_PASSWD "Arghya@19"

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

/*
*@brief Queus the commands, of type app_nodeData_t,
*       targeted to a Node. The maximum no of commands
        in the queue is limited by "APP_CONFIG_NODE_CMD_QUEUE_SIZE"
*/
QueueHandle_t app_nodeCommandQueue; 
QueueHandle_t app_nodeResponseQueue;

app_status_t app_meshHubInit(void);
app_status_t app_consolInit(void);
app_status_t app_consolRegisterNodeCmd(void);
app_status_t app_mqttClientInit();
void app_userInputInit();
#endif