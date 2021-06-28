
#include "app_main.h"
#include "esp_adc_cal.h"
#include "driver/timer.h"
//#define BLINK_GPIO                          GPIO_NUM_18
#define TRIAC_PIN                           GPIO_NUM_13
#define TIMER_INTERRUPT_INTERVAL            100  //in microseconds
#define DEFAULT_VREF                        1100
#define ADC_WIDTH_BIT                       ADC_WIDTH_BIT_13 //ADC_WIDTH_BIT_13 for ESP32_s2
#define TRIAC_OFF_PERIOD(period)            ((10-period)*10) 


timer_group_t timer_group_adc = TIMER_GROUP_0;
timer_idx_t timer_idx_adc = TIMER_1;
timer_config_t timer_config_adc = {
    .counter_en = TIMER_PAUSE,
    .counter_dir = TIMER_COUNT_UP,
    .alarm_en = TIMER_ALARM_EN,
    .auto_reload = TIMER_AUTORELOAD_EN,
    .divider = 80
};

static TaskHandle_t isrCollectAdcSampleTaskHandle = NULL;

esp_adc_cal_characteristics_t *adc_chars;

static bool IRAM_ATTR timer_isr_handler(void *args){
    //TOGGLE GPIO 18----------------------------------
    // if(gpio_get_level(BLINK_GPIO)==1)
    //     gpio_set_level(BLINK_GPIO, 0);
    // else
    //     gpio_set_level(BLINK_GPIO, 1);
    //----------------------------------------------------------------
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(isrCollectAdcSampleTaskHandle, &xHigherPriorityTaskWoken);
    if(xHigherPriorityTaskWoken==pdTRUE)
        portYIELD_FROM_ISR();
    return xHigherPriorityTaskWoken;
}

void isrCollectAdcSampleTask(void *pvParameters){
    uint16_t present_mv=0, raw=0;
    static uint16_t previous_mv = 0;
    static uint8_t regulate=0;
    static uint8_t state=0;
    static uint64_t triac_off_period = 0;
    while (1)
    {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        raw=adc1_get_raw(ADC1_CHANNEL_0);
        present_mv = esp_adc_cal_raw_to_voltage(raw, adc_chars);  
        switch(state){
            case 0:
                if(previous_mv>present_mv && present_mv>500){
                    uint8_t temp;            
                    if (xQueueReceive(device_FanRegulateQueue, &temp, 0) == pdTRUE){
                        regulate=temp;
                        //input_ledIndicator(temp);
                    }
                    if(regulate>10)
                        regulate=10;
                    if(regulate>0){
                        if(regulate<10)
                            gpio_set_level(TRIAC_PIN, 0);    
                        triac_off_period = TRIAC_OFF_PERIOD(regulate);
                        state++;
                    }
                } 
                break;               
            case 1:
                if(triac_off_period<=0){
                    gpio_set_level(TRIAC_PIN, 1);
                    state ++;
                }
                else if(triac_off_period>0)
                    --triac_off_period;
                break;
            case 2:
                if(previous_mv<present_mv)
                    state=0;
                break;
            default:
                gpio_set_level(TRIAC_PIN, 0);
                state = 0;
        }
        
        if(abs(previous_mv-present_mv)>5)
            previous_mv = present_mv;
    }

}

void timer_adc_initialise(void)
{
    // gpio_pad_select_gpio(BLINK_GPIO);
    // gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT_OUTPUT);
    gpio_pad_select_gpio(TRIAC_PIN);
    gpio_set_direction(TRIAC_PIN, GPIO_MODE_OUTPUT);

    //ADC1 channel 0 (GPIO 36)
    adc1_config_width(ADC_WIDTH_BIT);
    adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_2_5);
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_2_5, ADC_WIDTH_BIT, DEFAULT_VREF, adc_chars);

    device_FanRegulateQueue = xQueueCreate(2, sizeof(uint8_t));
    xTaskCreate(isrCollectAdcSampleTask, "isrCollectAdcSampleTask", 3000, NULL, configMAX_PRIORITIES -1, &isrCollectAdcSampleTaskHandle);


    timer_init(timer_group_adc, timer_idx_adc, &timer_config_adc);
    timer_set_counter_value(timer_group_adc, timer_idx_adc, 0);
    timer_set_alarm_value(timer_group_adc, timer_idx_adc, TIMER_INTERRUPT_INTERVAL);
    timer_isr_callback_add(timer_group_adc, timer_idx_adc,timer_isr_handler, (void *)NULL,ESP_INTR_FLAG_IRAM);
    timer_start(timer_group_adc,timer_idx_adc);
}
