#include "app_main.h"
#include "task.h"
void perform_taks1(void * pvParameters){
    int num = 0;
    while(1){
        if (xQueueReceive(task_queue, &num, 0) == pdTRUE)
             ++num;
        xQueueSend(task_queue, (void *)&num, 10);
        printf("After Task1: %d\n", num);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
void perform_taks2(void * pvParameters){
    int num = 0;
    while(1){
        if (xQueueReceive(task_queue, &num, 0) == pdTRUE)
            num += 5;
        xQueueSend(task_queue, (void *)&num, 10);
        printf("After Task2: %d\n", num);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
void task_manager(char* task){
    static TaskHandle_t xHandle[] = {NULL,NULL,NULL,NULL,NULL,NULL};
    printf("\n\nCommand: %s\n\n", task);
            if(!strcmp(task,"task1"))
            {
                if(xHandle[1]==NULL){
                    xTaskCreate(
                            &perform_taks1,
                            "Task1",
                            4000,
                            NULL,
                            1,
                            &xHandle[1]
                    );
            }
                else {
                    printf("Task-1 Terminated\n\r");
                    vTaskDelete(xHandle[1]);
                    xHandle[1] = NULL;
                }
            }
            else if(!strcmp(task,"task2"))
            {
                 if(xHandle[2]==NULL){
                    xTaskCreate(
                            &perform_taks2,
                            "Task2",
                            4000,
                            NULL,
                            1,
                            &xHandle[2]
                    );
                }
                else {
                    printf("Task-2 Terminated\n\r");
                    vTaskDelete(xHandle[2]);
                    xHandle[2] = NULL;
                }
            }
}