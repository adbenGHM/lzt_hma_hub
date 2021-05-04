#include "app_main.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_wifi.h"

#define DELAY                   10000  //in microseconds
#define BUTTON_GPIO             GPIO_NUM_15

#define MINIMUM_BUTTON_PRESS_PERIOD     100     //in miliseconds
#define MINIMUM_BUTTON_RELEASE_PERIOD   100     //in milliseconds
#define MAXIMUM_RELESE_PERIOD_BETWEEN_CONSICUTIVE_PRESS 800

#define NUM_OF_BUTTON_INPUTS    1

static uint64_t millis = 0;
typedef struct{
    gpio_num_t buttonGpioNum;
    uint8_t buttonPressedStateLogicLevel;  
    uint64_t eventMillis;
    uint8_t pressCount;
    uint8_t pressDetectionState;
}button_t;

typedef struct{
    gpio_num_t buttonGpioNum;
    uint8_t pressCount;
} button_details_t;

static button_t buttonElemetArray[NUM_OF_BUTTON_INPUTS];

button_t buttonElement0={
    .buttonGpioNum=BUTTON_GPIO,
    .buttonPressedStateLogicLevel=0,
    .eventMillis=0,
    .pressCount=0,
    .pressDetectionState=0
};


timer_group_t timer_group = TIMER_GROUP_0;
timer_idx_t timer_idx = TIMER_0;
timer_config_t timer_config = {
    .counter_en = TIMER_PAUSE,
    .counter_dir = TIMER_COUNT_UP,
    .alarm_en = TIMER_ALARM_EN,
    .auto_reload = TIMER_AUTORELOAD_EN,
    .divider = 80
};

static TaskHandle_t isrTakeInputTaskHandle = NULL;

uint64_t getMilis(void){
    return millis;
}
void onBoard(){
    printf("ON-BOARDING!!\n");
}
void takeActionOnInputTask(void* pvParameters){
    button_details_t button_details_receive;
    while(1){
        if(xQueueReceive(app_buttonDetailsQueue, &button_details_receive, 0)==pdTRUE)
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
static bool IRAM_ATTR timer_isr_handler(void *args){
    millis+=10;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(isrTakeInputTaskHandle, &xHigherPriorityTaskWoken);
    if(xHigherPriorityTaskWoken==pdTRUE)
        portYIELD_FROM_ISR();
    return xHigherPriorityTaskWoken;
}
void isrTakeInputTask(void* pvParameters){
    button_details_t button_details;
    while(1)
    {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);   
        for(int buttonIndex=0;buttonIndex<NUM_OF_BUTTON_INPUTS;buttonIndex++){
            switch(buttonElemetArray[buttonIndex].pressDetectionState){
                case 0:
                    if(gpio_get_level(buttonElemetArray[buttonIndex].buttonGpioNum)==buttonElemetArray[buttonIndex].buttonPressedStateLogicLevel){
                        buttonElemetArray[buttonIndex].eventMillis=getMilis();
                        buttonElemetArray[buttonIndex].pressDetectionState=1;
                    }
                break;
                case 1:
                    if(gpio_get_level(buttonElemetArray[buttonIndex].buttonGpioNum)==buttonElemetArray[buttonIndex].buttonPressedStateLogicLevel){
                        if((getMilis()-buttonElemetArray[buttonIndex].eventMillis)>MINIMUM_BUTTON_PRESS_PERIOD)
                            buttonElemetArray[buttonIndex].pressDetectionState=2; 
                    }
                    else{
                        buttonElemetArray[buttonIndex].pressDetectionState=5;  
                    }
                break;
                case 2:
                    if(gpio_get_level(buttonElemetArray[buttonIndex].buttonGpioNum)!=buttonElemetArray[buttonIndex].buttonPressedStateLogicLevel){
                        buttonElemetArray[buttonIndex].eventMillis=getMilis();
                        buttonElemetArray[buttonIndex].pressDetectionState=3;     
                    }
                break;
                case 3:
                    if(gpio_get_level(buttonElemetArray[buttonIndex].buttonGpioNum)!=buttonElemetArray[buttonIndex].buttonPressedStateLogicLevel){
                        if((getMilis()-buttonElemetArray[buttonIndex].eventMillis)>MINIMUM_BUTTON_RELEASE_PERIOD){
                            buttonElemetArray[buttonIndex].pressDetectionState=4;
                            buttonElemetArray[buttonIndex].eventMillis=getMilis();
                            buttonElemetArray[buttonIndex].pressCount=buttonElemetArray[buttonIndex].pressCount+1;
                        }
                    }
                    else{
                        buttonElemetArray[buttonIndex].pressDetectionState=5; 
                    }
                break;
                case 4:
                    if((getMilis()-buttonElemetArray[buttonIndex].eventMillis)>MAXIMUM_RELESE_PERIOD_BETWEEN_CONSICUTIVE_PRESS){                        
                            buttonElemetArray[buttonIndex].pressDetectionState=5;  
                    }
                    else if(gpio_get_level(buttonElemetArray[buttonIndex].buttonGpioNum)==buttonElemetArray[buttonIndex].buttonPressedStateLogicLevel){
                        buttonElemetArray[buttonIndex].pressDetectionState=0;    
                    }
                break;
                case 5:
                    if(buttonElemetArray[buttonIndex].pressCount>0){
                        button_details.buttonGpioNum = buttonElemetArray[buttonIndex].buttonGpioNum;
                        button_details.pressCount = buttonElemetArray[buttonIndex].pressCount;
                        xQueueSend(app_buttonDetailsQueue, &button_details, 10);
                        //     printf("\r\nPress Count : %d\r\n",buttonElemetArray[buttonIndex].pressCount); 
                    }
                    buttonElemetArray[buttonIndex].pressDetectionState=0; 
                    buttonElemetArray[buttonIndex].pressCount=0; 
                    break;
                default:
                    buttonElemetArray[buttonIndex].pressDetectionState=0; 
                    buttonElemetArray[buttonIndex].pressCount=0; 
            }
            
        }
    } 
}

void app_userInputInit()
{
    gpio_pad_select_gpio(BUTTON_GPIO);
    gpio_set_pull_mode(BUTTON_GPIO,GPIO_PULLUP_ONLY);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);

    buttonElemetArray[0]=buttonElement0;

    timer_init(timer_group, timer_idx, &timer_config);
    timer_set_counter_value(timer_group, timer_idx, 0);
    timer_set_alarm_value(timer_group, timer_idx, DELAY);
    timer_isr_callback_add(timer_group, timer_idx, (timer_isr_t)timer_isr_handler, (void *)NULL,ESP_INTR_FLAG_IRAM);
    timer_start(timer_group,timer_idx);

    xTaskCreate(&isrTakeInputTask, "TakeInputTask", 3000, NULL, configMAX_PRIORITIES - 1, &isrTakeInputTaskHandle);
    xTaskCreate(&takeActionOnInputTask, "TakeActionOnInputTask", 3000, NULL, configMAX_PRIORITIES - 2, &isrTakeInputTaskHandle);
    app_buttonDetailsQueue = xQueueCreate(5, sizeof(button_details_t));
}