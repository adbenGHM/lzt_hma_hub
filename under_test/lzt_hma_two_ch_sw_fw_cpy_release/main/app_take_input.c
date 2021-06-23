#include "app_input.h"

button_t buttonElement0={
    .buttonGpioNum=BUTTON_IN1,
    .buttonPressedStateLogicLevel=0,
    .eventMillis=0,
    .pressCount=0,
    .pressDetectionState=0
};
button_t buttonElement1={
    .buttonGpioNum=BUTTON_IN2,
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

uint64_t getMilis(void){
    return millis;
}

bool IRAM_ATTR timer_isr_handler(void *args){
    millis+=1;
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
            if(gpio_get_level(buttonElemetArray[buttonIndex].buttonGpioNum)==buttonElemetArray[buttonIndex].buttonPressedStateLogicLevel){
                buttonElemetArray[buttonIndex].eventMillis=getMilis();
                button_details.buttonGpioNum = buttonElemetArray[buttonIndex].buttonGpioNum;
                button_details.pressCount = 1;
                xQueueSend(app_buttonDetailsQueue, &button_details, 10);
            }
            if(((getMilis()-buttonElemetArray[buttonIndex].eventMillis) >= MINIMUM_BUTTON_PRESS_PERIOD) && buttonElemetArray[buttonIndex].pressCount > 0){
                button_details.pressCount = 7;
                xQueueSend(app_buttonDetailsQueue, &button_details, 10);
                buttonElemetArray[buttonIndex].pressCount=0;
            }   
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
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

    isrTakeInputTaskHandle = NULL;
    millis = 0;
    
    xTaskCreate(&isrTakeInputTask, "TakeInputTask", 3000, NULL, configMAX_PRIORITIES - 1, &isrTakeInputTaskHandle);
    app_buttonDetailsQueue = xQueueCreate(5, sizeof(button_details_t));
    xTaskCreate(&app_process_button_input_Task, "ProcessInputTask", 4000, NULL, configMAX_PRIORITIES - 1, NULL);

    initialise_timer();
}
void control_Ind_Led(uint32_t state){
    gpio_set_level(RELAY1_ON_IND_LED,state);
    gpio_set_level(RELAY1_OFF_IND_LED ,state);
    gpio_set_level(RELAY2_ON_IND_LED,state);
    gpio_set_level(RELAY2_OFF_IND_LED,state);
}
void app_InitIO(){
    gpio_config_t io_conf = {
        .intr_type = GPIO_PIN_INTR_DISABLE,
        //set as output mode
        .mode = GPIO_MODE_INPUT_OUTPUT,
        //bit mask of the pins that you want to set,e.g.GPIO18/19
        .pin_bit_mask = (1ULL << RELAY1_ON_IND_LED) | (1ULL << RELAY1_OFF_IND_LED) | ((1ULL << RELAY2_ON_IND_LED) | (1ULL << RELAY2_OFF_IND_LED) |  (1ULL << RELAY_OUT1) | (1ULL << RELAY_OUT2)),
        //disable pull-down mode
        .pull_down_en = 0,
        //disable pull-up mode
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
    //button 2
    gpio_set_level(RELAY1_OFF_IND_LED,1); //red
    gpio_set_level(RELAY1_ON_IND_LED,0); //green

    //button 1
    gpio_set_level(RELAY2_OFF_IND_LED,1); //red
    gpio_set_level(RELAY2_ON_IND_LED,0); //green

    gpio_set_direction(RELAY_OUT1, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(RELAY_OUT1,0);
    gpio_set_direction(RELAY_OUT1, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(RELAY_OUT2,0);
    
    gpio_pad_select_gpio(BUTTON_IN1);
    gpio_set_pull_mode(BUTTON_IN1,GPIO_PULLUP_ONLY);
    gpio_set_direction(BUTTON_IN1, GPIO_MODE_INPUT);

    gpio_pad_select_gpio(BUTTON_IN2);
    gpio_set_pull_mode(BUTTON_IN2,GPIO_PULLUP_ONLY);
    gpio_set_direction(BUTTON_IN2, GPIO_MODE_INPUT);
}
