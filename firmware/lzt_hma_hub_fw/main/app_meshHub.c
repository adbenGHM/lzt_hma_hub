#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"

#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "app_main.h"

static const char *TAG = "app_mesh";

static mesh_addr_t meshParentAddr;
static int meshLayer = -1;
static esp_netif_t *p_netifSta = NULL;
static bool isMeshConnected = false;

void meshScanDoneHandler(int num);

void meshTransmitTask(void *arg)
{
    app_nodeData_t nodeCmd;
    esp_err_t err;
    mesh_addr_t nodeAddress;
    mesh_data_t data;
    uint32_t qStatus;
    data.data = (uint8_t *)nodeCmd.data;
    data.size = APP_CONFIG_NODE_DATA_MAX_LEN;
    data.proto = MESH_PROTO_BIN;
    data.tos = MESH_TOS_P2P;

    while (true)
    {
        memset(nodeCmd.nodeId, '\0', sizeof(nodeCmd.nodeId));
        memset(nodeCmd.data, '\0', sizeof(nodeCmd.data));
        qStatus = xQueueReceive(app_nodeCommandQueue, &nodeCmd, portMAX_DELAY);
        if (qStatus == pdPASS)
        {
            if (6 == sscanf(nodeCmd.nodeId, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &nodeAddress.addr[0], &nodeAddress.addr[1], &nodeAddress.addr[2], &nodeAddress.addr[3], &nodeAddress.addr[4], &nodeAddress.addr[5]))
            {
                data.size = strlen((char *)nodeCmd.data);
                ESP_LOGI(TAG, "Node ID : " MACSTR ", Cmd : %s", MAC2STR(nodeAddress.addr), nodeCmd.data);
                err = esp_mesh_send(&nodeAddress, &data, MESH_DATA_P2P, NULL, 0);
                if (err)
                    ESP_LOGE(TAG, "[heap:%d][err:0x%x]Error Sending command to node\r\n", esp_get_minimum_free_heap_size(), err);
            }
        }
    }
    vTaskDelete(NULL);
}
void meshReceiveTask(void *arg)
{
    app_nodeData_t nodeResponse;
    esp_err_t err;
    mesh_addr_t from;
    mesh_data_t data;
    int flag = 0;
    data.data = (uint8_t *)nodeResponse.data;

    while (true)
    {
        data.size = APP_CONFIG_NODE_DATA_MAX_LEN;
        memset(nodeResponse.nodeId, '\0', sizeof(nodeResponse.nodeId));
        memset(nodeResponse.data, '\0', sizeof(nodeResponse.data));
        err = esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0);
        if (err != ESP_OK || !data.size)
        {
            ESP_LOGE(TAG, "err:0x%x, size:%d", err, data.size);
            continue;
        }
        sprintf(nodeResponse.nodeId, MACSTR, from.addr[0], from.addr[1], from.addr[2], from.addr[3], from.addr[4], from.addr[5]);
        ESP_LOGI(TAG, "NodeID : %s, Received Data : %s\r\n", nodeResponse.nodeId, nodeResponse.data);
        xQueueSend(app_nodeResponseQueue, &nodeResponse, 0);
    }
    vTaskDelete(NULL);
}

esp_err_t meshCommP2PStart(void)
{
    static bool isCommP2PStarted = false;
    if (!isCommP2PStarted)
    {
        isCommP2PStarted = true;
        xTaskCreate(meshTransmitTask, "MPTX", 3072, NULL, 5, NULL);
        xTaskCreate(meshReceiveTask, "MPRX", 3072, NULL, 5, NULL);
    }
    return ESP_OK;
}
void meshScanDoneHandler(int num)
{
    int i;
    int ie_len = 0;
    mesh_assoc_t assoc;
    mesh_assoc_t parent_assoc = {.layer = CONFIG_MESH_MAX_LAYER, .rssi = -120};
    wifi_ap_record_t record;
    wifi_ap_record_t parent_record = {
        0,
    };
    bool parent_found = false;
    mesh_type_t my_type = MESH_IDLE;
    int my_layer = -1;
    wifi_config_t parent = {
        0,
    };
    wifi_scan_config_t scan_config = {0};

    for (i = 0; i < num; i++)
    {
        esp_mesh_scan_get_ap_ie_len(&ie_len);
        esp_mesh_scan_get_ap_record(&record, &assoc);
        if (ie_len == sizeof(assoc))
        {
            ESP_LOGW(TAG,
                     "<MESH>[%d]%s, layer:%d/%d, assoc:%d/%d, %d, " MACSTR ", channel:%u, rssi:%d, ID<" MACSTR "><%s>",
                     i, record.ssid, assoc.layer, assoc.layer_cap, assoc.assoc,
                     assoc.assoc_cap, assoc.layer2_cap, MAC2STR(record.bssid),
                     record.primary, record.rssi, MAC2STR(assoc.mesh_id), assoc.encrypted ? "IE Encrypted" : "IE Unencrypted");
        }
        else
        {
            ESP_LOGI(TAG, "[%d]%s, " MACSTR ", channel:%u, rssi:%d", i,
                     record.ssid, MAC2STR(record.bssid), record.primary,
                     record.rssi);

            if (!strcmp(appConfig.wifiSsid, (char *)record.ssid))
            {
                parent_found = true;
                memcpy(&parent_record, &record, sizeof(record));
                my_type = MESH_ROOT;
                my_layer = MESH_ROOT_LAYER;
            }
        }
    }
    esp_mesh_flush_scan_result();
    if (parent_found)
    {
        /*
         * parent
         * Both channel and SSID of the parent are mandatory.
         */
        parent.sta.channel = parent_record.primary;
        memcpy(&parent.sta.ssid, &parent_record.ssid,
               sizeof(parent_record.ssid));
        parent.sta.bssid_set = 1;
        memcpy(&parent.sta.bssid, parent_record.bssid, 6);
        if (my_type == MESH_ROOT)
        {
            if (parent_record.authmode != WIFI_AUTH_OPEN)
            {
                memcpy(&parent.sta.password, appConfig.wifiPassword,
                       strlen(appConfig.wifiPassword));
            }
            ESP_LOGW(TAG, "<PARENT>%s, " MACSTR ", channel:%u, rssi:%d",
                     parent_record.ssid, MAC2STR(parent_record.bssid),
                     parent_record.primary, parent_record.rssi);
            ESP_ERROR_CHECK(esp_mesh_set_parent(&parent, NULL, my_type, my_layer));
        }
        else
        {
            ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(parent_record.authmode));
            if (parent_record.authmode != WIFI_AUTH_OPEN)
            {
                memcpy(&parent.sta.password, APP_CONFIG_MESH_AP_PASSWD,
                       strlen(APP_CONFIG_MESH_AP_PASSWD));
            }
            ESP_LOGW(TAG,
                     "<PARENT>%s, layer:%d/%d, assoc:%d/%d, %d, " MACSTR ", channel:%u, rssi:%d",
                     parent_record.ssid, parent_assoc.layer,
                     parent_assoc.layer_cap, parent_assoc.assoc,
                     parent_assoc.assoc_cap, parent_assoc.layer2_cap,
                     MAC2STR(parent_record.bssid), parent_record.primary,
                     parent_record.rssi);
            ESP_ERROR_CHECK(esp_mesh_set_parent(&parent, (mesh_addr_t *)&parent_assoc.mesh_id, my_type, my_layer));
        }
    }
    else
    {
        esp_wifi_scan_stop();
        scan_config.show_hidden = 1;
        scan_config.scan_type = WIFI_SCAN_TYPE_PASSIVE;
        ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, 0));
    }
}

void meshEventHandler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    mesh_addr_t id = {
        0,
    };
    static int last_layer = 0;
    wifi_scan_config_t scan_config = {0};

    switch (event_id)
    {
    case MESH_EVENT_STARTED:
    {
        esp_mesh_get_id(&id);
        ESP_LOGI(TAG, "<MESH_EVENT_STARTED>ID:" MACSTR "", MAC2STR(id.addr));
        meshLayer = esp_mesh_get_layer();
        ESP_ERROR_CHECK(esp_mesh_set_self_organized(0, 0));
        esp_wifi_scan_stop();
        /* mesh softAP is hidden */
        scan_config.show_hidden = 1;
        scan_config.scan_type = WIFI_SCAN_TYPE_PASSIVE;
        ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, 0));
    }
    break;
    case MESH_EVENT_STOPPED:
    {
        ESP_LOGI(TAG, "<MESH_EVENT_STOPPED>");
        meshLayer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_CHILD_CONNECTED:
    {
        mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
        ESP_LOGI(TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, " MACSTR "",
                 child_connected->aid,
                 MAC2STR(child_connected->mac));
    }
    break;
    case MESH_EVENT_CHILD_DISCONNECTED:
    {
        mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
        ESP_LOGI(TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, " MACSTR "",
                 child_disconnected->aid,
                 MAC2STR(child_disconnected->mac));
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_ADD:
    {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new);
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE:
    {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new);
    }
    break;
    case MESH_EVENT_NO_PARENT_FOUND:
    {
        mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
        ESP_LOGI(TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                 no_parent->scan_times);
    }
    /* TODO handler for the failure */
    break;
    case MESH_EVENT_PARENT_CONNECTED:
    {
        mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
        esp_mesh_get_id(&id);
        meshLayer = connected->self_layer;
        memcpy(&meshParentAddr.addr, connected->connected.bssid, 6);
        ESP_LOGI(TAG,
                 "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:" MACSTR "%s, ID:" MACSTR "",
                 last_layer, meshLayer, MAC2STR(meshParentAddr.addr),
                 esp_mesh_is_root() ? "<ROOT>" : (meshLayer == 2) ? "<layer2>"
                                                                   : "",
                 MAC2STR(id.addr));
        last_layer = meshLayer;
        isMeshConnected = true;
        if (esp_mesh_is_root())
        {
            esp_netif_dhcpc_start(p_netifSta);
        }
        meshCommP2PStart();
    }
    break;
    case MESH_EVENT_PARENT_DISCONNECTED:
    {
        mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
        ESP_LOGI(TAG,
                 "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                 disconnected->reason);
        meshLayer = esp_mesh_get_layer();
        if (disconnected->reason == WIFI_REASON_ASSOC_TOOMANY)
        {
            esp_wifi_scan_stop();
            scan_config.show_hidden = 1;
            scan_config.scan_type = WIFI_SCAN_TYPE_PASSIVE;
            ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, 0));
        }
    }
    break;
    case MESH_EVENT_LAYER_CHANGE:
    {
        mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
        meshLayer = layer_change->new_layer;
        ESP_LOGI(TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
                 last_layer, meshLayer,
                 esp_mesh_is_root() ? "<ROOT>" : (meshLayer == 2) ? "<layer2>"
                                                                   : "");
        last_layer = meshLayer;
    }
    break;
    case MESH_EVENT_ROOT_ADDRESS:
    {
        mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
        ESP_LOGI(TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:" MACSTR "",
                 MAC2STR(root_addr->addr));
    }
    break;
    case MESH_EVENT_TODS_STATE:
    {
        mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
        ESP_LOGI(TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
    }
    break;
    case MESH_EVENT_ROOT_FIXED:
    {
        mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
        ESP_LOGI(TAG, "<MESH_EVENT_ROOT_FIXED>%s",
                 root_fixed->is_fixed ? "fixed" : "not fixed");
    }
    break;
    case MESH_EVENT_SCAN_DONE:
    {
        mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
        ESP_LOGI(TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
                 scan_done->number);
        meshScanDoneHandler(scan_done->number);
    }
    break;
    default:
        ESP_LOGD(TAG, "event id:%d", event_id);
        break;
    }
}

void ipEventHandler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
}

app_status_t app_meshHubInit()
{

   ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&p_netifSta, NULL));
    /* wifi initialization */
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ipEventHandler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
    /* mesh initialization */
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &meshEventHandler, NULL));
    /* mesh enable IE crypto */
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
#if CONFIG_MESH_IE_CRYPTO_FUNCS
    /* modify IE crypto key */
    ESP_ERROR_CHECK(esp_mesh_set_ie_crypto_funcs(&g_wifi_default_mesh_crypto_funcs));
    ESP_ERROR_CHECK(esp_mesh_set_ie_crypto_key(CONFIG_MESH_IE_CRYPTO_KEY, strlen(CONFIG_MESH_IE_CRYPTO_KEY)));
#else
    /* disable IE crypto */
    ESP_LOGI(TAG, "<Config>disable IE crypto");
    ESP_ERROR_CHECK(esp_mesh_set_ie_crypto_funcs(NULL));
#endif
    /* mesh ID */
    memcpy((uint8_t *)&cfg.mesh_id, APP_CONFIG_MESH_ID, 6);
    /* router */
    app_loadConfig();
    cfg.channel = CONFIG_MESH_CHANNEL;
    cfg.router.ssid_len = strlen(appConfig.wifiSsid);
    memcpy((uint8_t *)&cfg.router.ssid, appConfig.wifiSsid, cfg.router.ssid_len);
    memcpy((uint8_t *)&cfg.router.password, appConfig.wifiPassword,
           strlen(appConfig.wifiPassword));
    /* mesh softAP */
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(CONFIG_MESH_AP_AUTHMODE));
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *)&cfg.mesh_ap.password, APP_CONFIG_MESH_AP_PASSWD,
           strlen(APP_CONFIG_MESH_AP_PASSWD));
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    /* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());
    ESP_LOGI(TAG, "mesh starts successfully, heap:%d\n", esp_get_free_heap_size());

    return APP_STATUS_OK;
}
void app_meshInit(){
    app_nodeCommandQueue = xQueueCreate(APP_CONFIG_NODE_CMD_QUEUE_SIZE, sizeof(app_nodeData_t));
    app_nodeResponseQueue = xQueueCreate(APP_CONFIG_NODE_RESPONSE_QUEUE_SIZE, sizeof(app_nodeData_t));
    app_meshHubInit();
    app_consolRegisterNodeCmd();
    app_consolInit();    
    app_mqttClientInit();
    ESP_LOGI(TAG,"Initialization Done\r\n");
    app_nodeData_t nodeResponse;
    uint8_t state=0;
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    while(1){
        sprintf(nodeResponse.data,"{\"d1\" : \"%d\"}",state);
        xQueueSend(app_nodeResponseQueue, &nodeResponse, 0);   
        state=!state;
        vTaskDelay(10000 / portTICK_PERIOD_MS); 
    }
}