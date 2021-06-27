#include "esp_sntp.h"
#include "app_main.h"

#define APP_CONFIG_TIMEZONE "UTC-05:30"
#define DURATION_BETWEEN_CHECK 1000  //in ms

const char *TAG = "sntp";
const char dayNames[7][3] ={"Su", "Mo", "Tu", "We", "Th","Fr","Sa"};
void check_current_time_Task(void* pvParameters){
    time_t now;
    struct tm timeinfo;
    while(1){
        time(&now);
        localtime_r(&now, &timeinfo);
        for (int i = 0; i < MAX_NUM_ID; ++i){
            for (int j = 0; j < 7; ++j){
                if(strcmp(device[i].action,"delete") != 0){
                    if (strcmp(device[i].days[j],dayNames[timeinfo.tm_wday])==0)
                    {
                        if(device[i].startTime.hr == timeinfo.tm_hour && device[i].startTime.min == timeinfo.tm_min && device[i].startTime.sec == timeinfo.tm_sec){
                            printf("\r\ntime sec : %d , %d \r\n",timeinfo.tm_sec,device[i].startTime.sec);
                            printf("\r\nSchedule on : %s\r\n",device[i].channelKey);
                            app_nodeData_t nodeCmd;
                            memset(nodeCmd.data,'\0',sizeof(nodeCmd.data));
                            sprintf(nodeCmd.data, "{\"%s\":\"1\"}",device[i].channelKey);
                            xQueueSend(app_nodeCommandQueue, &nodeCmd, 0);
                            vTaskDelay(100 / portTICK_PERIOD_MS);
                        }
                        if(device[i].endTime.hr == timeinfo.tm_hour && device[i].endTime.min == timeinfo.tm_min && device[i].endTime.sec == timeinfo.tm_sec){
                            printf("\r\nSchedule off : %s\r\n",device[i].channelKey);
                            app_nodeData_t nodeCmd;
                            memset(nodeCmd.data,'\0',sizeof(nodeCmd.data));
                            sprintf(nodeCmd.data, "{\"%s\":\"0\"}",device[i].channelKey);
                            xQueueSend(app_nodeCommandQueue, &nodeCmd, 0);
                            vTaskDelay(100 / portTICK_PERIOD_MS);
                        }
                    }
                }
            }
        }
        vTaskDelay(950 / portTICK_PERIOD_MS);
        
    }
    vTaskDelete(NULL);
}
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI("SNTP", "Notification of a time synchronization event");
}
void initialize_sntp(void)
{
    ESP_LOGI("SNTP", "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();
}
app_status_t device_syncTime(void)
{
    char strftime_buf[64];
    time_t now;
    struct tm timeinfo;
    int retry = 0;
    const int retry_count = 10;
    initialize_sntp();
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI("OBTAIN TIME", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    if(retry==retry_count)
        return APP_STATUS_ERROR;
        
    setenv("TZ", APP_CONFIG_TIMEZONE, 1);
    tzset();
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in India is: %s", strftime_buf);
    
    return APP_STATUS_OK;
}