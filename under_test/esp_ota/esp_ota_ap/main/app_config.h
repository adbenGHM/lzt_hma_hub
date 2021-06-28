#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

// #define APP_CONFIG_TIMEZONE "UTC-05:30"
// #define APP_CONFIG_HTTP_DATA_SERVER_URL "http://192.168.0.105:8099/uplink_data"

#define APP_CONFIG_AP_WIFI_SSID "esp32_ap"
#define APP_CONFIG_AP_WIFI_PASS "esp@connect"
#define APP_CONFIG_AP_WIFI_CHANNEL 1 //between 1 to 13
#define APP_CONFIG_AP_WIFI_MAX_STA_CONN 5
// #define APP_CONFIG_AP_IP_ADDRESS {10,10,10,0}
// #define APP_CONG_AP_GATEWAY_IP_ADDRESS  {10,10,10,0}
// #define APP_CONFIG_AP_NET_MASK {255,255,255,0}

// #define APP_CONFIG_MAX_NO_OF_NODES_SUPPORTED 20
// #define APP_CONFIG_NODE_ADDRESS_LENGTH 4   //in bytes
// #define APP_CONFIG_MAX_NODE_DATA_LEGTH 100 //in bytes
// #define APP_CONFIG_NODE_DATA_QUEUE_SIZE 10


// #define APP_CONFIG_MAX_SIZE_OF_NODE_DATA_JSON_STR (APP_CONFIG_NODE_ADDRESS_LENGTH + APP_CONFIG_MAX_NODE_DATA_LEGTH + 64 + 80)

// #define APP_CONFIG_ESP_MAXIMUM_WIFI_RETRY 5

// #define APP_CONFIG_NVS_NAMESPACE "configNamespace"
// struct appConf
// {
//     char wifiSsid[32];     //should not be changed
//     char wifiPassword[64]; //should not be changed
//     char nodeAddressList[APP_CONFIG_MAX_NO_OF_NODES_SUPPORTED][APP_CONFIG_NODE_ADDRESS_LENGTH + 1];
// } appConfig;

#endif