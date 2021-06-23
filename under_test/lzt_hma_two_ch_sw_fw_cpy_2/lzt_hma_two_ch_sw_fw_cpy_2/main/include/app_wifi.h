
#include "esp_wifi.h"
#include "freertos/event_groups.h"

//==============================MUST BE SAME FOR BOTH NODE AND HUB==============================

#define APP_CONFIG_MESH_AP_PASSWD "Arghya@123"
// #define APP_CONFIG_MESH_ROUTER_SSID "Soumyanetra"
// #define APP_CONFIG_MESH_ROUTER_PASSWD "Twamoshi"

static const uint8_t APP_CONFIG_MESH_ID[6] = {0x77, 0x77, 0x77, 0x77, 0x77, 0x77};
//===============================================================================================

void start_webserver(void);