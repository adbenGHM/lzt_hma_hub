
#include "app_main.h"
static const char *TAG = "wifi softAP";

void softApApp()
{
    app_wifiApInit();
    
}
void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    softApApp();
    printf("\r\n1This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n2This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n3This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n4This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n5This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n6This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n7This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n8This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n9This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n10This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n11This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n12This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n13This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\nThis is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n106This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n105This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n104This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n103This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n102This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n101This is similar to SpaceX vertical landing rocket\r\n");
    printf("\r\n107This is similar to SpaceX vertical landing rocket\r\n");
    ota_example_init();
    //esp_ota_begin()
}
