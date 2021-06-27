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
//#include "esp_system.h"


#include "lwip/err.h"
#include "lwip/sys.h"

#define APP_CONFIG_NODE_ID_LEN 20 //size of node ID in Bytes

#define APP_CONFIG_NODE_CMD_QUEUE_SIZE 10 //maximum no of node commads that could be queued
#define APP_CONFIG_NODE_RESPONSE_QUEUE_SIZE 10 //maximum no fo repose from node that can be queued 

#define APP_CONFIG_MQTT_BROKER_URL "mqtt://3.128.241.99"
#define APP_CONFIG_MQTT_BROKER_PORT "1883"

//do not change
#define APP_MAX_MQTT_DATA_LENGTH (APP_CONFIG_NODE_DATA_MAX_LEN+APP_CONFIG_NODE_ID_LEN+100)

#define APP_CONFIG_NODE_DATA_MAX_LEN 200 //maximum size of node data in bytes

#define MAX_NUM_ID 10

typedef struct{
    char nodeId[APP_CONFIG_NODE_ID_LEN+1]; //mac ID of the target Node
    char data[APP_CONFIG_NODE_DATA_MAX_LEN+1];
    uint8_t pck_type : 1;
}app_nodeData_t;
typedef enum
{
    APP_STATUS_ERROR_NVS=-3,
    APP_STATUS_ERROR_STA_MODE_CONNECTION_FAILED=-2,
    APP_STATUS_ERROR = -1,
    APP_STATUS_OK = 0,
} app_status_t;

typedef struct appConf
{
    char wifiSsid[32];     //should not be changed
    char wifiPassword[64]; //should not be changed
    bool startMesh;
} appConf;

typedef struct statusConf
{
    char nodeID[25];
    uint8_t state[5+1];
} statusConf;

struct storeBlock{
    appConf appConfig;
    statusConf statusConfig;
} storeBlock;

typedef struct{
    uint8_t hr;
    uint8_t min;
    uint8_t sec;
} timeDiv_t;

typedef struct {
    char action[20];         //Specifies Mode (create/Modify/delete)
    char channelKey[10];     //Target Device
    timeDiv_t startTime;  //Schedule Start Time
    timeDiv_t endTime;    //Schedule End Time
    char days[7][3];      //Days of Week (sun-mon-tue-wed-thu-fri-sat)
} scheduleDetails_t;

scheduleDetails_t device[MAX_NUM_ID];
/*
*@brief Queus the commands, of type app_nodeData_t,
*       targeted to a Node. The maximum no of commands
        in the queue is limited by "APP_CONFIG_NODE_CMD_QUEUE_SIZE"
*/


// app_status_t app_meshHubInit(void);


QueueHandle_t app_nodeCommandQueue; 
QueueHandle_t app_nodeResponseQueue;
//QueueHandle_t device_offlineIndQueue;

void check_current_time_Task(void *);
app_status_t app_consolInit(void);
app_status_t app_consolRegisterNodeCmd(void);
app_status_t app_mqttClientInit();
app_status_t app_wifiApInit(void);
app_status_t app_saveConfig(void);
app_status_t app_loadConfig(void);
app_status_t app_saveStatus(app_nodeData_t);
app_status_t device_syncTime(void);
void app_userInputInit();
void device_multicastInit(void);
//void app_meshInit(void);
app_status_t app_wifiStaInit(void);

void processContolMqttData(char*, uint16_t);
void initialise_timer(void);
void app_InitIO(void);
void input_taskManager(void);
void control_Ind_Led(uint32_t);

//uint8_t isOFFLINE;
//void sendMultiicast(app_nodeData_t);
//TODO : notification for wifi not connected
//TODO : disconnected from internet
#endif