/* Mesh Manual Networking Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "app_main.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "driver/timer.h"
static const char *TAG = "app_main";
char fw_version[]="version_1.1";



void app_main(void)
{
    uint8_t configMode=0;
    uint8_t ledState=0;
    app_InitIO();
    printf("\r\n*********************************************************************\r\n");
    printf("\r*  Version [%s]\r\n",fw_version);
    printf("\r\n*********************************************************************\r\n");
    esp_err_t ret = nvs_flash_init();
    app_status_t resp;   
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(ret);
    /* tcpip initialization */
    ESP_ERROR_CHECK(esp_netif_init());
    /* event initialization */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    app_nodeCommandQueue = xQueueCreate(APP_CONFIG_NODE_CMD_QUEUE_SIZE, sizeof(app_nodeData_t));
    app_nodeResponseQueue = xQueueCreate(APP_CONFIG_NODE_RESPONSE_QUEUE_SIZE, sizeof(app_nodeData_t));
    input_taskManager();
    app_loadConfig();
    //storeBlock.appConfig.startMesh = false;
    app_nodeData_t nodeCmd;
    for(int i=1;i<sizeof(storeBlock.statusConfig.state);i++){
        memset(nodeCmd.data,'\0',sizeof(nodeCmd.data));
        sprintf(nodeCmd.data, "{\"d%d\":\"%d\"}",i,storeBlock.statusConfig.state[i]);
        printf("\r\nfrom flash %s\r\n",nodeCmd.data);
        xQueueSend(app_nodeCommandQueue,&nodeCmd,0);
    }
    if(storeBlock.appConfig.startMesh == false){
        ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
        configMode=1;
        storeBlock.appConfig.startMesh = true;
        app_saveConfig();
        app_wifiApInit();
        ota_init();
    }
    else{
        if(strlen((char*)storeBlock.appConfig.wifiSsid)>0 && strlen((char*)storeBlock.appConfig.wifiPassword)>0){
            ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
            resp=app_wifiStaInit();
            if(resp != APP_STATUS_OK)
            {
                // while(1){
                //     printf("Error\r\n");
                //     vTaskDelay(5000/portTICK_PERIOD_MS);
                // }
            }
        }
        else{
            printf("\r\nPlease configure the device\r\n");
        }
        
        
        
        //device_multicastInit();
        app_mqttClientInit();
        app_userInputInit();
        xTaskCreate(check_current_time_Task,"check_current_time_Task",4000,NULL,configMAX_PRIORITIES - 1,NULL);
        device_syncTime();
    }
    ESP_LOGI(TAG,"Initialization Done\r\n");
    
    while(true){
        if(configMode){
            control_Ind_Led(ledState);  
            ledState=!ledState;  
            vTaskDelay(500/portTICK_PERIOD_MS);
        }        
        else
            vTaskDelay(portMAX_DELAY);      
    }
    
}