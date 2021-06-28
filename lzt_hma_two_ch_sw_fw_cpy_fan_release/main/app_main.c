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

// void app_main(void)
// {
//     ESP_ERROR_CHECK(nvs_flash_init());
//     /* tcpip initialization */
//     ESP_ERROR_CHECK(esp_netif_init());
//     /* event initialization */
//     ESP_ERROR_CHECK(esp_event_loop_create_default());

//     app_nodeCommandQueue = xQueueCreate(APP_CONFIG_NODE_CMD_QUEUE_SIZE, sizeof(app_nodeData_t));
//     app_nodeResponseQueue = xQueueCreate(APP_CONFIG_NODE_RESPONSE_QUEUE_SIZE, sizeof(app_nodeData_t));
//     app_meshHubInit();
//     app_consolRegisterNodeCmd();
//     app_consolInit();    
//     app_mqttClientInit();
//     ESP_LOGI(TAG,"Initialization Done\r\n");
//     app_nodeData_t nodeResponse;
//     uint8_t state=0;
//     vTaskDelay(10000 / portTICK_PERIOD_MS);
//     while(1){
//         sprintf(nodeResponse.data,"{\"d1\" : \"%d\"}",state);
//         xQueueSend(app_nodeResponseQueue, &nodeResponse, 0);   
//         state=!state;
//         vTaskDelay(10000 / portTICK_PERIOD_MS); 
//     }
// }




void app_main(void)
{
    uint8_t configMode=0;
    uint8_t ledState=0;
    app_InitIO();
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

    app_loadConfig();
    if(appConfig.startMesh == false){
        ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
        configMode=1;
        appConfig.startMesh = true;
        app_saveConfig();
        app_wifiApInit();
    }
    else{
        if(strlen((char*)appConfig.wifiSsid)>0 && strlen((char*)appConfig.wifiPassword)>0){
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
        
        app_nodeCommandQueue = xQueueCreate(APP_CONFIG_NODE_CMD_QUEUE_SIZE, sizeof(app_nodeData_t));
        app_nodeResponseQueue = xQueueCreate(APP_CONFIG_NODE_RESPONSE_QUEUE_SIZE, sizeof(app_nodeData_t));
        timer_adc_initialise();
        input_taskManager();
        app_mqttClientInit();
        app_userInputInit();
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