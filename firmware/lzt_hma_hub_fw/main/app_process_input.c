#include "app_main.h"
void app_process_input_Task(void* pvParameters){
    button_details_t button_details_receive;
    while(1){
        if(xQueueReceive(app_buttonDetailsQueue, &button_details_receive, portMAX_DELAY)==pdTRUE)
        {
            switch(button_details_receive.pressCount){
                case 1:
                    printf("%d button pressed 1\r\n",button_details_receive.buttonGpioNum);
                    if(button_details_receive.buttonGpioNum==GPIO_NUM_2){
                        gpio_set_level(GPIO_NUM_6,1);
                        gpio_set_level(GPIO_NUM_7,1);
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        //gpio_set_level(GPIO_NUM_6,0);
                        //gpio_set_level(GPIO_NUM_7,0);
                    }else{
                        gpio_set_level(GPIO_NUM_4,1);
                        gpio_set_level(GPIO_NUM_5,1); 
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
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