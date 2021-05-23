#include "app_main.h"

#define AUTO_CUTOFF_MILLIS 10000

TaskHandle_t auto_Cutoff_Task_Handle = NULL;

void auto_Cutoff_Task(void* pvParameters){
    uint8_t swt=*(uint8_t*)pvParameters;
    uint64_t startMillis = getMilis();
    
    while(1){
        if((getMilis()-startMillis) >= AUTO_CUTOFF_MILLIS){
            app_nodeData_t nodeCmd;
            memset(nodeCmd.data,'\0',sizeof(nodeCmd.data));
            if(swt==2)
                strcpy(nodeCmd.data, "{\"d2\" : \"0\"}");
            else
                strcpy(nodeCmd.data, "{\"d1\" : \"0\"}");
            xQueueSend(app_nodeCommandQueue, &nodeCmd, 0);
            break;
        }
        if(swt==2 && gpio_get_level(RELAY_OUT2)==0)
            break;
        else if(swt==1 && gpio_get_level(RELAY_OUT1)==0)
            break;
        else if(gpio_get_level(RELAY_OUT1)==0 && gpio_get_level(RELAY_OUT2)==0)
            break;
        vTaskDelay(10/ portTICK_PERIOD_MS);
    }
    auto_Cutoff_Task_Handle = NULL;
    vTaskDelete(NULL);
}
void app_process_cmd_input_Task(void* pvParameters){
    app_nodeData_t nodeCmd;
    app_nodeData_t nodeResponse;
    uint8_t swt;
    memset(nodeCmd.data,'\0',sizeof(nodeCmd.data));
    while(1){
        if(xQueueReceive(app_nodeCommandQueue, &nodeCmd, portMAX_DELAY)== pdTRUE){
            
            char *ptr = strstr(nodeCmd.data, "d2");
            char *colon = strstr(nodeCmd.data, ":");
            uint8_t state=0;
            if(ptr != NULL && colon != NULL){
                if(gpio_get_level(GPIO_NUM_12) == 0)
                {
                    state=*(ptr + strlen("d2\" : ")+1);
                    state -= 48;        //ASCII of 0: 48
                    gpio_set_level(GPIO_NUM_6,(uint8_t)!state);      //Control red led
                    gpio_set_level(GPIO_NUM_7,(uint8_t)state);       //Control green led
                    gpio_set_level(GPIO_NUM_13,(uint8_t)state);      //Control RELAY 2
                    sprintf(nodeResponse.data,"{\"d2\" : \"%d\"}",state);
                    if((uint8_t)state == 1){
                        swt = 2;
                        xTaskCreate(&auto_Cutoff_Task, "auto_Cutoff_Task", 1000, (void*)&swt, 2, &auto_Cutoff_Task_Handle);
                    }
                }
                else
                    sprintf(nodeResponse.data,"{\"d2\" : \"0\"}");
            }
            else{
                ptr = strstr(nodeCmd.data, "d1");
                if(ptr != NULL && colon != NULL)
                {
                    if(gpio_get_level(GPIO_NUM_13) == 0)
                    {
                        
                        state=*(ptr + strlen("d1\" : ")+1);
                        state -= 48;        //ASCII of 0: 48
                        gpio_set_level(GPIO_NUM_4,(uint8_t)!state);    //Control red led
                        gpio_set_level(GPIO_NUM_5,(uint8_t)state);     //Control green led 
                        gpio_set_level(GPIO_NUM_12,(uint8_t)state);    //Control RELAY 1
                        sprintf(nodeResponse.data,"{\"d1\" : \"%d\"}",state);
                        if((uint8_t)state == 1){
                            swt = 1;
                            xTaskCreate(&auto_Cutoff_Task, "auto_Cutoff_Task", 1000, (void *)&swt, 2, &auto_Cutoff_Task_Handle);
                        }
                    }
                    else
                        sprintf(nodeResponse.data,"{\"d1\" : \"0\"}");
                }
            }
            memset(nodeCmd.data,'\0',sizeof(nodeCmd.data));
            xQueueSend(app_nodeResponseQueue, &nodeResponse, 0);
        }
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
                        if(gpio_get_level(RELAY_OUT2) == 1){  //if GREEN ON
                            strcpy(nodeCmd.data, "{\"d2\" : \"0\"}");
                        }
                        else{
                            strcpy(nodeCmd.data, "{\"d2\" : \"1\"}");
                        }
                        xQueueSend(app_nodeCommandQueue, &nodeCmd, 0);
                        
                    }else if(button_details_receive.buttonGpioNum==BUTTON_IN1){
                        if(gpio_get_level(RELAY_OUT1) == 1){  //if GREEN ON
                            strcpy(nodeCmd.data, "{\"d1\" : \"0\"}");
                        }
                        else{
                            strcpy(nodeCmd.data, "{\"d1\" : \"1\"}");
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
                    gpio_set_level(GPIO_NUM_6,0);      
                    gpio_set_level(GPIO_NUM_7,0); 
                    app_saveConfig();
                    vTaskDelay(1000/portTICK_PERIOD_MS);
                    esp_restart();
                    break;    
            }
        }
        
    }
    vTaskDelete(NULL);
}