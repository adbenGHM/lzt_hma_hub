#include "app_main.h"
#include "mqtt_client.h"
static const char *TAG = "MQTT_CLIENT";
//{"NodeId" : "24:0a:c4:af:4c:fc", "data" : {"sw1": "1", "sw2" :"1"}}
static char *strnchr(char *s, unsigned char c, size_t len)
{
    while (len && *s != c)
    {
        s++;
        --len;
    }

    return len ? s : NULL;
}
void processMqttData(char *dataStr, uint16_t dataLen)
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
    ESP_LOGI(TAG, "Node ID : %s , Data : %s\r\n", nodeCmd.nodeId, nodeCmd.data);
    xQueueSend(app_nodeCommandQueue, &nodeCmd, 0);
}
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "24:1a:c4:af:4c:fc/control", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
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
        processMqttData(event->data, event->data_len);
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
            //sprintf((char*)mqttStr,"{\"NodeId\" : \"%s\", \"data\" : %s}",nodeResponse.nodeId,nodeResponse.data);
            sprintf((char*)mqttStr,"{\"NodeId\" : \"%s\", \"Data\" : %s}","24:1a:c4:af:4c:fc",nodeResponse.data);
            printf("\r\n");
            printf((char*)mqttStr);
            printf("\r\n");
            esp_mqtt_client_publish(client, "24:1a:c4:af:4c:fc/monitor", (char*)mqttStr, 0, 1, 0);
        }
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
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://3.128.241.99",
        .port = 1883,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
    xTaskCreate(mqttPublish, "MQTTPUB", 3072, client, 4, NULL);
    return APP_STATUS_OK;
}