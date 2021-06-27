#include "app_main.h"
#include <string.h>
#include "mqtt_client.h"
#include "app_input.h"
#include "cJSON.h"
static const char *TAG = "MQTT_CLIENT";
//{"NodeId" : "24:0a:c4:af:4c:fc", "data" : {"sw1": "1", "sw2" :"1"}}
static char macID[18];
static char publishTopic[100];
static char subscribeTopic[100];
static char subscribeScheduleTopic[100];
static char *strnchr(char *s, unsigned char c, size_t len)
{
    while (len && *s != c)
    {
        s++;
        --len;
    }

    return len ? s : NULL;
}
void setTime(uint8_t id, char* value, char cmd){
    char *ptr;
    uint8_t pos = 0;
    char buf[3];
    while(value){
        ptr = strnchr(value, ':', sizeof(value));
        memset(buf,'\0',sizeof(buf));
        strncpy(buf, value,sizeof(value-ptr)-2);
        if(ptr != NULL){
            if(cmd == 's'){
                if(pos == 0)
                    device[id].startTime.hr = (uint8_t)atoi(buf);
                else
                    device[id].startTime.min = (uint8_t)atoi(buf);
            }
            else{
                if(pos == 0)
                    device[id].endTime.hr = (uint8_t)atoi(buf);
                else
                    device[id].endTime.min = (uint8_t)atoi(buf);
            }
            value += (sizeof(ptr)-1);
            ++pos;
        }
        else{
            if(cmd == 's')
                device[id].startTime.sec = (uint8_t)atoi(buf);
            else
                device[id].endTime.sec = (uint8_t)atoi(buf);
            break;
        }
    }
}
void setDays(uint8_t id, char* value){
    char *ptr;
    uint8_t pos=0;
    for (int i = 0; i < 7;++i){
         memset(device[id].days[i], '\0', sizeof(device[id].days[i]));
    }
    while (value)
    {
        ptr = strnchr(value, ',', sizeof(value));
        if (ptr != NULL)
        {
            strncpy(device[id].days[pos], value, sizeof(device[id].days[pos]) - 1);
            ++pos;
            value += (sizeof(ptr) - 1);
        }
        else{
            strcpy(device[id].days[pos], value);
            break;
        }
    }
    
}
void processScheduleMqttData(char *dataStr, uint16_t dataLen){
    //cJSON* request_json = cJSON_Parse(dataStr);
    cJSON* item = cJSON_Parse(dataStr);//cJSON_GetObjectItem(request_json,"schedules");
    uint8_t id=0;
    char* value;
    //printf("json id type %d\r\n",cJSON_GetObjectItem(item, "id")->type);
    //id=7;
    id = (cJSON_GetObjectItem(item, "id")->valueint);
    value = cJSON_GetObjectItem(item, "action")->valuestring;
    memset(device[id].action,'\0',sizeof(device[id].action));
    strcpy(device[id].action, value);
    value = cJSON_GetObjectItem(item, "channelKey")->valuestring;
    memset(device[id].channelKey,'\0',sizeof(device[id].channelKey));
    strcpy(device[id].channelKey, value);
    value = cJSON_GetObjectItem(item, "startTime")->valuestring;
    setTime(id,value,'s');
    value = cJSON_GetObjectItem(item, "endtime")->valuestring;
    setTime(id,value,'e');
    value = cJSON_GetObjectItem(item, "days")->valuestring;
    setDays(id,value);
    printf("\n\nID: %d\n",id);
    printf("Action: %s\n",device[id].action);
    printf("Channel Key: %s\n",device[id].channelKey);
    printf("Start: %d:%d:%d\n",device[id].startTime.hr,device[id].startTime.min,device[id].startTime.sec);
    printf("End: %d:%d:%d\n",device[id].endTime.hr,device[id].endTime.min,device[id].endTime.sec);
    for (int i = 0; i < 7; ++i)
        if(device[id].days[i][0] != '\0')
            printf("Day: %s\n",device[id].days[i]);
            
    cJSON_Delete(item);
}
void processContolMqttData(char *dataStr, uint16_t dataLen)
{
    app_nodeData_t nodeCmd;
    memset(nodeCmd.nodeId, '\0', sizeof(nodeCmd.nodeId));
    memset(nodeCmd.data, '\0', sizeof(nodeCmd.data));
    char *p_pos;
    uint16_t remainingLength = dataLen;
    p_pos = strnstr(dataStr, "NodeId", remainingLength);
    if (p_pos == NULL)
    {
        ESP_LOGE(TAG, "Error processing command \"%.*s\"\r\n", dataLen, dataStr);
        return;
    }
    p_pos = p_pos + strlen("NodeId");
    remainingLength = dataLen - (p_pos - dataStr);
    p_pos = strnchr(p_pos, ':', remainingLength);
    if (p_pos == NULL)
    {
        ESP_LOGE(TAG, "Error processing command \"%.*s\"\r\n", dataLen, dataStr);
        return;
    }
    p_pos++;
    remainingLength = dataLen - (p_pos - dataStr);
    p_pos = strnchr(p_pos, '\"', remainingLength);
    if (p_pos == NULL)
    {
        ESP_LOGE(TAG, "Error processing command \"%.*s\"\r\n", dataLen, dataStr);
        return;
    }
    p_pos += 1;
    remainingLength = dataLen - (p_pos - dataStr);
    char *p_tempPos = p_pos;
    p_pos = strnchr(p_pos, '\"', remainingLength);
    if (p_pos == NULL)
    {
        ESP_LOGE(TAG, "Error processing command \"%.*s\"\r\n", dataLen, dataStr);
        return;
    }
    if ((p_pos - p_tempPos) > APP_CONFIG_NODE_ID_LEN)
    {
        ESP_LOGE(TAG, "NodeId too long \"%.*s\"\r\n", dataLen, dataStr);
        return;
    }
    strncpy(nodeCmd.nodeId, p_tempPos, p_pos - p_tempPos);
    p_pos += 1;
    remainingLength = dataLen - (p_pos - dataStr);
    p_pos = strnchr(p_pos, '{', remainingLength);
    if (p_pos == NULL)
    {
        ESP_LOGE(TAG, "Error processing command \"%.*s\"\r\n", dataLen, dataStr);
        return;
    }
    p_tempPos = p_pos;
    p_pos += 1;
    remainingLength = dataLen - (p_pos - dataStr);
    p_pos = strnchr(p_pos, '}', remainingLength);
    if (p_pos == NULL)
    {
        ESP_LOGE(TAG, "Error processing command \"%.*s\"\r\n", dataLen, dataStr);
        return;
    }
    p_pos += 1;
    if ((p_pos - p_tempPos) > APP_CONFIG_NODE_DATA_MAX_LEN)
    {
        ESP_LOGE(TAG, "Data too long \"%.*s\"\r\n", dataLen, dataStr);
        return;
    }
    strncpy(nodeCmd.data, p_tempPos, p_pos - p_tempPos);
    ESP_LOGI(TAG, "NodeId : %s , Data : %s\r\n", nodeCmd.nodeId, nodeCmd.data);
    app_saveStatus(nodeCmd);
    storeBlock.statusConfig.state[1] = 0;
    storeBlock.statusConfig.state[2] = 0;
    storeBlock.statusConfig.state[3] = 0;
    storeBlock.statusConfig.state[4] = 0;
    storeBlock.statusConfig.state[5] = 0;
    app_loadConfig();
    printf("\n%d\t%d", 1, storeBlock.statusConfig.state[1]);
    printf("\n%d\t%d", 2, storeBlock.statusConfig.state[2]);
    printf("\n%d\t%d", 3, storeBlock.statusConfig.state[3]);
    printf("\n%d\t%d", 4, storeBlock.statusConfig.state[4]);
    printf("\n%d\t%d", 5, storeBlock.statusConfig.state[5]);
    // app_loadConfig();
    // printf("%s",appConfig.wifiSsid);
    //saveState(nodeCmd);
    xQueueSend(app_nodeCommandQueue, &nodeCmd, 0);
    //app_process_cmd_input_Task();
}
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    app_nodeData_t nodeResponse;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, subscribeTopic, 0);
        printf("\r\nNode Subscribed to topic [%s]\r\n",subscribeTopic);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, subscribeScheduleTopic, 0);
        printf("\r\nNode Subscribed to topic [%s]\r\n",subscribeScheduleTopic);

        sprintf(nodeResponse.data,"{\"d1\" : \"%d\",\"d2\" : \"%d\"}",gpio_get_level(RELAY_OUT1),gpio_get_level(RELAY_OUT2));
        xQueueSend(app_nodeResponseQueue, &nodeResponse, 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        printf("\nOFFLINE MODE ACTIVATED!!\n");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        char* ptr = strstr(event->topic,"/");
        if(*(ptr+1) == 'c') //control
            processContolMqttData(event->data, event->data_len);
        else //schedule
            processScheduleMqttData(event->data, event->data_len);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}
static void mqttPublish(void *arg)
{   
    app_nodeData_t nodeResponse;
    uint32_t qStatus;
    esp_mqtt_client_handle_t client=(esp_mqtt_client_handle_t)arg;
    uint8_t mqttStr[APP_MAX_MQTT_DATA_LENGTH+1];
    while (true)
    {
        memset(nodeResponse.nodeId, '\0', sizeof(nodeResponse.nodeId));
        memset(nodeResponse.data, '\0', sizeof(nodeResponse.data));
        memset(mqttStr,'\0', sizeof(mqttStr));
        qStatus = xQueueReceive(app_nodeResponseQueue, &nodeResponse, portMAX_DELAY);
        if (qStatus == pdPASS)
        {
            sprintf((char*)mqttStr,"{\"NodeId\" : \"%s\", \"Data\" : %s, \"Type\" : %d}",macID,nodeResponse.data,nodeResponse.pck_type);
            printf("\r\nNode publishing [%s]to topic [%s]\r\n",mqttStr,publishTopic);
            esp_mqtt_client_publish(client, publishTopic, (char*)mqttStr, 0, 1, 1);
        }
    
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

app_status_t app_mqttClientInit()
{
    uint8_t derived_mac_addr[6] = {0};
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://3.128.241.99",
        .port = 1883,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_read_mac(derived_mac_addr, ESP_MAC_WIFI_STA));
    sprintf(macID, "%x:%x:%x:%x:%x:%x", derived_mac_addr[0], derived_mac_addr[1], derived_mac_addr[2],
                                        derived_mac_addr[3], derived_mac_addr[4], derived_mac_addr[5]);
    memcpy(publishTopic,macID,sizeof(macID));
    memcpy(subscribeTopic,macID,sizeof(macID));
    memcpy(subscribeScheduleTopic,macID,sizeof(macID));
    ESP_LOGI(TAG, "sent subscribe successful, MAC STA ID: %s", macID);
    
    strcat(publishTopic,"/monitor");
    printf("Mqtt [Monitor] publish topic : %s\r\n", publishTopic);
    
    strcat(subscribeTopic,"/control");
    printf("Mqtt [Control] subscribe topic : %s\r\n", subscribeTopic);
    
    strcat(subscribeScheduleTopic,"/schedule/#");
    printf("Mqtt [Schedule] subscribe topic : %s\r\n", subscribeTopic);
    
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
    xTaskCreate(mqttPublish, "MQTTPUB", 4098, client, 4, NULL);
    return APP_STATUS_OK;
}