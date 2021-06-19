#ifndef __APP_INPUT__
#define __APP_INPUT__


#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/timer.h"
#include "driver/gpio.h"

#define DELAY                   1000  //in microseconds


#define MINIMUM_BUTTON_PRESS_PERIOD     20     //in miliseconds
#define MINIMUM_BUTTON_RELEASE_PERIOD   20     //in milliseconds
#define MAXIMUM_RELESE_PERIOD_BETWEEN_CONSICUTIVE_PRESS 500

#define NUM_OF_BUTTON_INPUTS    2

//==============================================================================================
#define BUTTON_IN1              GPIO_NUM_1
#define BUTTON_IN2              GPIO_NUM_2
#define RELAY_OUT1              GPIO_NUM_12
#define RELAY_OUT2              GPIO_NUM_13
#define RELAY1_ON_IND_LED       GPIO_NUM_5
#define RELAY1_OFF_IND_LED      GPIO_NUM_4
#define RELAY2_ON_IND_LED       GPIO_NUM_7
#define RELAY2_OFF_IND_LED      GPIO_NUM_6
//==============================================================================================
uint64_t millis;

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


button_t buttonElemetArray[NUM_OF_BUTTON_INPUTS];

TaskHandle_t isrTakeInputTaskHandle;

QueueHandle_t app_buttonDetailsQueue;

uint64_t getMilis(void);
void app_process_button_input_Task(void*);
void check_current_time_Task(void *);
//void app_process_cmd_input_Task(void* pvParameters);

#endif