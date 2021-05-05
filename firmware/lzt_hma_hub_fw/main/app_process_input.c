#include "app_main.h"
void app_process_input_Task(void* pvParameters){
    button_details_t button_details_receive;
    while(1){
        if(xQueueReceive(app_buttonDetailsQueue, &button_details_receive, portMAX_DELAY)==pdTRUE)
        {
            switch(button_details_receive.pressCount){
                case 1:
                    printf("1 is pressed\r\n");
                    break;
                case 2:
                    printf("2 is pressed\r\n");
                    break;
                case 3:
                    printf("3 is pressed\r\n");
                    break;
                case 4:
                    printf("4 is pressed\r\n");
                    break;
                case 5:
                    appConfig.startMesh = false;
                    app_saveConfig();
                    esp_restart();
                    break;    
            }
        }
        
    }
    vTaskDelete(NULL);
}