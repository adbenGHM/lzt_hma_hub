#include "app_main.h"
#include "app_input.h"

void app_process_cmd_input_Task(void* pvParameters){
    app_nodeData_t nodeCmd;
    app_nodeData_t nodeResponse;
    memset(nodeCmd.data,'\0',sizeof(nodeCmd.data));
    while(1){
        xQueueReceive(app_nodeCommandQueue, &nodeCmd, portMAX_DELAY);
        char *ptr = strstr(nodeCmd.data, "d2");
        char *colon = strstr(nodeCmd.data, ":");
        char state;
        if(ptr != NULL && colon != NULL){
            strncpy(&state, ptr + strlen("d2\":")+1,1);
            state -= 48;        //ASCII of 0: 48
            gpio_set_level(RELAY2_OFF_IND_LED,(uint8_t)!state);      //Control red led
            gpio_set_level(RELAY2_ON_IND_LED,(uint8_t)state);       //Control green led
            gpio_set_level(RELAY_OUT2,(uint8_t)state);      //Control RELAY 
            storeBlock.statusConfig.state[2]=state;
            app_saveConfig();
            sprintf(nodeResponse.data,"{\"d1\" : \"%d\",\"d2\" : \"%d\"}",gpio_get_level(RELAY_OUT1),state);
        }
        else{
            ptr = strstr(nodeCmd.data, "d1");
            if(ptr != NULL && colon != NULL)
            {
                strncpy(&state, ptr + strlen("d1\":")+1,1);
                state -= 48;        //ASCII of 0: 48
                gpio_set_level(RELAY1_OFF_IND_LED,(uint8_t)!state);    //Control red led
                gpio_set_level(RELAY1_ON_IND_LED,(uint8_t)state);     //Control green led 
                gpio_set_level(RELAY_OUT1,(uint8_t)state);    //Control RELAY 1
                storeBlock.statusConfig.state[1]=state;
                app_saveConfig();
                sprintf(nodeResponse.data,"{\"d1\" : \"%d\",\"d2\" : \"%d\"}",state,gpio_get_level(RELAY_OUT2));
            }
        }
        memset(nodeCmd.data,'\0',sizeof(nodeCmd.data));
        nodeCmd.pck_type = 0;
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
                    if(button_details_receive.buttonGpioNum==BUTTON_IN2){
                        if(button_details_receive.pressDurationMillis<MINIMUM_PRESS_HOLD_PERIOD){
                            if(gpio_get_level(RELAY_OUT2) == 1){  //if GREEN ON
                                strcpy(nodeCmd.data, "{\"d2\":\"0\"}");
                            }
                            else{
                                strcpy(nodeCmd.data, "{\"d2\":\"1\"}");
                            }
                            xQueueSend(app_nodeCommandQueue, &nodeCmd, 0);
                        }
                        else{
                            storeBlock.appConfig.startMesh = false;
                            gpio_set_level(GPIO_NUM_4,0);      
                            gpio_set_level(GPIO_NUM_5,0); 
                            gpio_set_level(GPIO_NUM_6,0);      
                            gpio_set_level(GPIO_NUM_7,0); 
                            app_saveConfig();
                            vTaskDelay(1000/portTICK_PERIOD_MS);
                            esp_restart();
                        }
                        
                    }
                    else if(button_details_receive.buttonGpioNum==BUTTON_IN1){
                        if(button_details_receive.pressDurationMillis<MINIMUM_PRESS_HOLD_PERIOD){
                            if(gpio_get_level(RELAY_OUT1) == 1){  //if GREEN ON
                                strcpy(nodeCmd.data, "{\"d1\":\"0\"}");
                            }
                            else{
                                strcpy(nodeCmd.data, "{\"d1\":\"1\"}");
                            }
                            xQueueSend(app_nodeCommandQueue, &nodeCmd, 0);
                        }
                        else{
                            storeBlock.appConfig.startMesh = false;
                            gpio_set_level(GPIO_NUM_4,0);      
                            gpio_set_level(GPIO_NUM_5,0); 
                            gpio_set_level(GPIO_NUM_6,0);      
                            gpio_set_level(GPIO_NUM_7,0); 
                            app_saveConfig();
                            vTaskDelay(1000/portTICK_PERIOD_MS);
                            esp_restart();
                        }
                    }
                    break;
                default:
                    break;    
            }
        }
        
    }
    vTaskDelete(NULL);
}
void input_taskManager(void){
    xTaskCreate(&app_process_cmd_input_Task, "app_process_cmd_input_Task", 4000, NULL, configMAX_PRIORITIES - 1, NULL);
}