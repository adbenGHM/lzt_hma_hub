#include "app_main.h"
#include "driver/gpio.h"
#include "driver/timer.h"

#define DELAY                   100000  //in microseconds
#define BUTTON_GPIO             GPIO_NUM_15
#define DEBOUNCE_DELAY          50      //in miliseconds
#define MINIMUM_HOLD_PERIOD     500     //in miliseconds
#define MAXIMUM_HOLD_PERIOD     1000    //in miliseconds
#define COUNT_TO_ONBOARD        5
uint64_t millis = 0;

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
static bool IRAM_ATTR timer_isr_handler(void *args){
    millis+=100;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(isrTakeInputTaskHandle, &xHigherPriorityTaskWoken);
    if(xHigherPriorityTaskWoken==pdTRUE)
        portYIELD_FROM_ISR();
    return xHigherPriorityTaskWoken;
}
void isrTakeInputTask(void* pvParameters){
    static uint64_t inputTime = 0;
    static int previousReading=1;
    static int presentReading=1;
    static uint8_t state = 0;
    static uint8_t count = 0;
    while(1)
    {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        switch(state){
            case 0:
                presentReading = gpio_get_level(BUTTON_GPIO);
                if(presentReading!=previousReading)
                {
                    inputTime = getMilis();
                    previousReading = presentReading;
                    state = 1;
                }
                else
                    state = 0;
                break;
            case 1: 
                if( ((getMilis()-inputTime)>=MINIMUM_HOLD_PERIOD) && ((getMilis()-inputTime)<=MAXIMUM_HOLD_PERIOD) ){
                    if(presentReading!=gpio_get_level(BUTTON_GPIO))
                    {
                        if(presentReading==0)
                            ++count;
                        printf("\n\nCount: %d\n\n", count);
                        state = 0;
                    }
                    else
                        state = 1;
                }
                else
                {
                    if( ((getMilis()-inputTime)>MAXIMUM_HOLD_PERIOD) && (presentReading == gpio_get_level(BUTTON_GPIO)) ){
                        count = 0;
                        state = 0;
                    }
                    else if( ((getMilis()-inputTime)<MINIMUM_HOLD_PERIOD) && (presentReading!=gpio_get_level(BUTTON_GPIO)) ){
                        count = 0;
                        state = 0;
                    }
                    else
                        state = 1;
                }

                if(count == COUNT_TO_ONBOARD){
                    onBoard();
                    previousReading = 1;
                    count = 0;
                    state = 0;
                }
                break;
            default:
                state = 0;
        }
    }
}
void app_userInputInit()
{
    gpio_pad_select_gpio(BUTTON_GPIO);
    gpio_set_pull_mode(BUTTON_GPIO,GPIO_PULLUP_ONLY);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);

    timer_init(timer_group, timer_idx, &timer_config);
    timer_set_counter_value(timer_group, timer_idx, 0);
    timer_set_alarm_value(timer_group, timer_idx, DELAY);
    timer_isr_callback_add(timer_group, timer_idx, (timer_isr_t)timer_isr_handler, (void *)NULL,ESP_INTR_FLAG_IRAM);
    timer_start(timer_group,timer_idx);

    xTaskCreate(&isrTakeInputTask, "TakeInputTask", 3000, NULL, configMAX_PRIORITIES - 1, &isrTakeInputTaskHandle);
}