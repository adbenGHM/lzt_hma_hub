#include "app_main.h"
#include "driver/timer.h"
#include "esp_wifi.h"

#define DELAY                   10000  //in microseconds


#define MINIMUM_BUTTON_PRESS_PERIOD     100     //in miliseconds
#define MINIMUM_BUTTON_RELEASE_PERIOD   100     //in milliseconds
#define MAXIMUM_RELESE_PERIOD_BETWEEN_CONSICUTIVE_PRESS 800

#define NUM_OF_BUTTON_INPUTS    2

static uint64_t millis = 0;
typedef struct{
    gpio_num_t buttonGpioNum;
    uint8_t buttonPressedStateLogicLevel;  
    uint64_t eventMillis;
    uint8_t pressCount;
    uint8_t pressDetectionState;
}button_t;


static button_t buttonElemetArray[NUM_OF_BUTTON_INPUTS];

static button_t buttonElement0={
    .buttonGpioNum=BUTTON_IN1,
    .buttonPressedStateLogicLevel=0,
    .eventMillis=0,
    .pressCount=0,
    .pressDetectionState=0
};
static button_t buttonElement1={
    .buttonGpioNum=BUTTON_IN2,
    .buttonPressedStateLogicLevel=0,
    .eventMillis=0,
    .pressCount=0,
    .pressDetectionState=0
};

static timer_group_t timer_group = TIMER_GROUP_0;
static timer_idx_t timer_idx = TIMER_0;
static timer_config_t timer_config = {
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



bool IRAM_ATTR timer_isr_handler(void *args){
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
void initialise_timer(){
    timer_init(timer_group, timer_idx, &timer_config);
    timer_set_counter_value(timer_group, timer_idx, 0);
    timer_set_alarm_value(timer_group, timer_idx, DELAY);
    timer_isr_callback_add(timer_group, timer_idx, (timer_isr_t)timer_isr_handler, (void *)NULL,ESP_INTR_FLAG_IRAM);
    timer_start(timer_group,timer_idx);
}

void app_userInputInit()
{

    buttonElemetArray[0]=buttonElement0;  
    buttonElemetArray[1]=buttonElement1;   

    xTaskCreate(&isrTakeInputTask, "TakeInputTask", 3000, NULL, configMAX_PRIORITIES - 1, &isrTakeInputTaskHandle);
    app_buttonDetailsQueue = xQueueCreate(5, sizeof(button_details_t));
    xTaskCreate(&app_process_button_input_Task, "ProcessInputTask", 4000, NULL, configMAX_PRIORITIES - 1, NULL);

    initialise_timer();
}