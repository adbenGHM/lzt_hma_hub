#include "app_main.h"

void app_process_cmd_input_Task(void* pvParameters){
    app_nodeData_t nodeCmd;
    app_nodeData_t nodeResponse;
    memset(nodeCmd.data,'\0',sizeof(nodeCmd.data));
    while(1){
        xQueueReceive(app_nodeCommandQueue, &nodeCmd, portMAX_DELAY);
        char *ptr = strstr(nodeCmd.data, "d1");
        char *colon = strstr(nodeCmd.data, ":");
        char state;
        if(ptr != NULL && colon != NULL){
            strncpy(&state, ptr + strlen("d1\":")+1,1);
            state -= 48;        //ASCII of 0: 48
            gpio_set_level(RELAY1_OFF_IND_LED,(uint8_t)!state);      //Control red led
            gpio_set_level(RELAY1_ON_IND_LED,(uint8_t)state);       //Control green led
            gpio_set_level(RELAY_OUT1,(uint8_t)state);      //Control RELAY 2
            sprintf(nodeResponse.data,"{\"d1\" : \"%d\"}",state);
        }
        
        memset(nodeCmd.data,'\0',sizeof(nodeCmd.data));
        xQueueSend(app_nodeResponseQueue, &nodeResponse, 0);  
    } 
}

void app_process_button_input_Task(void* pvParameters){
    button_details_t button_details_receive;
    while(1){
        if(xQueueReceive(app_buttonDetailsQueue, &button_details_receive, portMAX_DELAY)==pdTRUE)
        {
            app_nodeData_t nodeCmd;
            memset(nodeCmd.data,'\0',sizeof(nodeCmd.data));
            switch(button_details_receive.pressCount){
                case 1:
                    printf("Button [%d] pressed Once\r\n",button_details_receive.buttonGpioNum);
                    if(button_details_receive.buttonGpioNum==BUTTON_IN1){
                        if(gpio_get_level(RELAY_OUT1) == 1){  //if GREEN ON
                            strcpy(nodeCmd.data, "{\"d1\":\"0\"}");
                        }
                        else{
                            strcpy(nodeCmd.data, "{\"d1\":\"1\"}");
                        }
                        xQueueSend(app_nodeCommandQueue, &nodeCmd, 0);
                    }
                    break;
                case 2:
                    printf("Button [%d] pressed Twice\r\n",button_details_receive.buttonGpioNum);
                    break;
                case 3:
                    printf("Button [%d] pressed Thrice\r\n",button_details_receive.buttonGpioNum);
                    break;
                case 4:
                    printf("Button [%d] pressed 4 times\r\n",button_details_receive.buttonGpioNum);
                    break;
                case 5:
                    printf("Button [%d] pressed 5 times\r\n",button_details_receive.buttonGpioNum);
                    appConfig.startMesh = false;
                    gpio_set_level(GPIO_NUM_4,0);      
                    gpio_set_level(GPIO_NUM_5,0); 
                    app_saveConfig();
                    vTaskDelay(1000/portTICK_PERIOD_MS);
                    esp_restart();
                    break;    
            }
        }
        
    }
    vTaskDelete(NULL);
}