#include "app_main.h"

void app_process_input_takeAction(){
    app_nodeData_t nodeCmd;
    app_nodeData_t nodeResponse;
    xQueueReceive(app_nodeCommandQueue, &nodeCmd, 0);
    char *ptr = strstr(nodeCmd.data, "d2");
    char *colon = strstr(nodeCmd.data, ":");
    char state;
    if(ptr != NULL && colon != NULL){
        strncpy(&state, ptr + strlen("d2\" : ")+1,1);
        state -= 48;        //ASCII of 0: 48
        if(state == 0)
            gpio_set_level(GPIO_NUM_6,1);      //TURN ON RED
        else
            gpio_set_level(GPIO_NUM_6,0);      //TURN OFF RED
        gpio_set_level(GPIO_NUM_7,(uint8_t)state);          //CONTROL GREEN
        gpio_set_level(GPIO_NUM_13,(uint8_t)state);         //CONTROL RELAY 2
        sprintf(nodeResponse.data,"{\"d2\" : \"%d\"}",state);
    }
    else{
        ptr = strstr(nodeCmd.data, "d1");
        if(ptr != NULL && colon != NULL)
        {
            strncpy(&state, ptr + strlen("d1\" : ")+1,1);
            state -= 48;        //ASCII of 0: 48
            if(state == 0)
                gpio_set_level(GPIO_NUM_4,1);      //TURN ON RED
            else
                gpio_set_level(GPIO_NUM_4,0);      //TURN OFF RED
            gpio_set_level(GPIO_NUM_5,(uint8_t)state);          //TURN OFF GREEN
            gpio_set_level(GPIO_NUM_12,(uint8_t)state);         //CONTROL RELAY 1
            sprintf(nodeResponse.data,"{\"d1\" : \"%d\"}",state);
        }
    }
    xQueueSend(app_nodeResponseQueue, &nodeResponse, 0);   
}

void app_process_input_Task(void* pvParameters){
    button_details_t button_details_receive;
    while(1){
        if(xQueueReceive(app_buttonDetailsQueue, &button_details_receive, portMAX_DELAY)==pdTRUE)
        {
            app_nodeData_t nodeCmd;
            switch(button_details_receive.pressCount){
                case 1:
                    printf("%d button pressed 1\r\n",button_details_receive.buttonGpioNum);
                    if(button_details_receive.buttonGpioNum==GPIO_NUM_2){
                        if(gpio_get_level(GPIO_NUM_7) == 1)  //if GREEN ON
                            strcpy(nodeCmd.data, "{\"d2\" : \"0\"}");
                        else
                           strcpy(nodeCmd.data, "{\"d2\" : \"1\"}");
                        xQueueSend(app_nodeCommandQueue, &nodeCmd, 0);
                        // gpio_set_level(GPIO_NUM_6,1);
                        // gpio_set_level(GPIO_NUM_7,1);
                        // vTaskDelay(1000 / portTICK_PERIOD_MS);
                        //gpio_set_level(GPIO_NUM_6,0);
                        //gpio_set_level(GPIO_NUM_7,0);
                    }else{
                        if(gpio_get_level(GPIO_NUM_5) == 1) //if GREEN ON
                            strcpy(nodeCmd.data, "{\"d1\" : \"0\"}");
                        else
                            strcpy(nodeCmd.data, "{\"d1\" : \"1\"}");
                        xQueueSend(app_nodeCommandQueue, &nodeCmd, 0);
                        // gpio_set_level(GPIO_NUM_4,1);
                        // gpio_set_level(GPIO_NUM_5,1); 
                        //vTaskDelay(1000 / portTICK_PERIOD_MS);
                        //gpio_set_level(GPIO_NUM_4,0);
                        //gpio_set_level(GPIO_NUM_5,0);   
                    }
                    break;
                case 2:
                    printf("%d button pressed 2\r\n",button_details_receive.buttonGpioNum);
                    break;
                case 3:
                    printf("%d button pressed 3\r\n",button_details_receive.buttonGpioNum);
                    break;
                case 4:
                    printf("%d button pressed 4\r\n",button_details_receive.buttonGpioNum);
                    break;
                case 5:
                    printf("%d button pressed 5\r\n",button_details_receive.buttonGpioNum);
                    appConfig.startMesh = false;
                    app_saveConfig();
                    esp_restart();
                    break;    
            }
        }
        
    }
    vTaskDelete(NULL);
}