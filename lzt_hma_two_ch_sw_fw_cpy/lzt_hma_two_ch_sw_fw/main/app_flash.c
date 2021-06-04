#include "app_main.h"
#include "nvs_flash.h"
#define APP_CONFIG_NVS_NAMESPACE "configNamespace"
app_status_t app_saveConfig()
{
    nvs_handle nvsHandle;
    esp_err_t err;
    size_t required_size;
    app_status_t returnStatus;
    required_size = sizeof(appConfig);
    char* clearArr[required_size];
   
    err = nvs_open(APP_CONFIG_NVS_NAMESPACE, NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK)
    {
        printf("\r\nError app_saveoCnfig : nvs_open\r\n");
        returnStatus=APP_STATUS_ERROR_NVS;
        goto _exit_point;
    }

    memset(clearArr,'\0',required_size);
    err = nvs_set_blob(nvsHandle, APP_CONFIG_NVS_NAMESPACE, clearArr, required_size);
    if (err != ESP_OK)
    {
        printf("\r\nError app_saveoCnfig : clearng nvs\r\n");
        returnStatus=APP_STATUS_ERROR_NVS;
        goto _exit_point;
    }


    err = nvs_set_blob(nvsHandle, APP_CONFIG_NVS_NAMESPACE, (char *)&appConfig, required_size);
    if (err != ESP_OK)
    {
        printf("\r\nError app_saveoCnfig : nvs_set_blob\r\n");
        returnStatus=APP_STATUS_ERROR_NVS;
        goto _exit_point;
    }

    err = nvs_commit(nvsHandle);
    if (err != ESP_OK)
    {
        printf("\r\nError Loading NVS : nvs_commit\r\n");
        returnStatus=APP_STATUS_ERROR_NVS;
        goto _exit_point;
    }
    returnStatus=APP_STATUS_OK;
    _exit_point:	   
        nvs_close(nvsHandle);
        return returnStatus;
}
app_status_t app_loadConfig()
{
    size_t required_size = 0; // value will default to 0, if not set yet in NVS
    esp_err_t err;
    nvs_handle nvsHandle;
    app_status_t returnStatus;

    err = nvs_open(APP_CONFIG_NVS_NAMESPACE, NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK)
    {
        printf("\r\nError app_loadConfig : nvs_open\r\n");
        returnStatus=APP_STATUS_ERROR_NVS;
        goto _exit_point;
    }

    err = nvs_get_blob(nvsHandle, APP_CONFIG_NVS_NAMESPACE, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        printf("\r\nError app_loadConfig : nvs_get_blob\r\n");
        returnStatus=APP_STATUS_ERROR_NVS;
        goto _exit_point;
    }

    if (required_size > 0)
    {
        memset(&appConfig,'\0',sizeof(appConfig));
        err = nvs_get_blob(nvsHandle, APP_CONFIG_NVS_NAMESPACE, (char *)&appConfig, &required_size);
        if (err != ESP_OK)
        {
            printf("\r\nError app_loadConfig : nvs_get_blob 2\r\n");
            returnStatus=APP_STATUS_ERROR_NVS;
            goto _exit_point;
        }
    }
    returnStatus=APP_STATUS_OK;
    _exit_point:	   
        nvs_close(nvsHandle);
        return returnStatus;
}

