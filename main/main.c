#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "common.h"
#include "ble.h"
#include "sensors.h"
#include "ota.h"
#include "esp_mac.h"


static const char* LOG_TAG = "MAIN";
void loadNvsIntoRAM();
void _loadStrValue(nvs_handle_t nvsHandle, 
    const char* key, char *value, size_t val_len, char *defaultVal);
void _loadBoolValue(nvs_handle_t nvsHandle, 
    const char* key, bool *value, bool defaultVal);

void app_main(void) 
{
    /* Initialize NVS */
    loadNvsIntoRAM();

    /* Initialize BLE and pin to Core 0 */
    configure_ble();
    nimble_port_freertos_init(ble_host_task);
    xTaskCreate(vTaskSendNotification, "vTaskSendNotification", 4096, NULL, 2, &xHandle);
    if (wifiEnable) wifi_config();
    run_mqtt();
    initSpi();
    xTaskCreate(vTaskReadBME280, "vTaskReadBME280", 4096, NULL, 3, &xHandle);
    #ifdef CONFIG_OTA_ENABLE
    xTaskCreate(ota_task, "vTaskSendNotification", 4096, NULL, 3, &xHandle);
    #endif
}

void loadNvsIntoRAM()
{
    /* NVS: Initialize */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Open NVS */
    nvs_handle_t nvsHandle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvsHandle));

    /* Read Values */
    _loadStrValue(nvsHandle, "client_cert", client_cert_pem, sizeof(client_cert_pem), "");
    _loadStrValue(nvsHandle, "client_key", client_key_pem, sizeof(client_key_pem), "");
    _loadStrValue(nvsHandle, "server_cert", server_cert_pem, sizeof(server_cert_pem), "");

    printf("client_cert: %s", client_cert_pem);
    printf("client_key_pem: %s", client_key_pem);
    printf("server_cert_pem: %s", server_cert_pem);

    _loadStrValue(nvsHandle, "wifi_ssid", ssid, sizeof(ssid), CONFIG_ESP_WIFI_SSID);
    _loadStrValue(nvsHandle, "deviceName", deviceName, MAX_DEVICE_NAME_LENGTH, CONFIG_ESP_BT_DEFAULT_NAME);
    _loadStrValue(nvsHandle, "wifi_psk", psk, WIFI_PSK_MAX, CONFIG_ESP_WIFI_PASSWORD);
    _loadStrValue(nvsHandle, "mqttEndpt", mqttEndpt, MAX_MQTT_ENDPT_LENGTH, "mqtts://mqtt.matthewtsai.uk:1883");
    _loadStrValue(nvsHandle, "tz", tz, 28, CONFIG_TIMEZONE);

    _loadBoolValue(nvsHandle, "wifiEnable", &wifiEnable, true);
    _loadBoolValue(nvsHandle, "rfidEnable", &rfidEnable, false);
    _loadBoolValue(nvsHandle, "tempEnable", &tempEnable, false);
    _loadBoolValue(nvsHandle, "pressureEnable", &pressureEnable, false);
    _loadBoolValue(nvsHandle, "humidityEnable", &humidityEnable, false);

    /* Set Env Vars */
    setenv("TZ", tz, 1);
    tzset();

    /* Load MAC addr */
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(macAddr, 18, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); 
    // nvs_commit(nvsHandle);
    nvs_close(nvsHandle);
}

void _loadStrValue(nvs_handle_t nvsHandle, const char* key, char *value, size_t val_len, char *defaultVal)
{
    esp_err_t err;
    err = nvs_get_str(nvsHandle, key, value, &val_len);
    ESP_LOGI(LOG_TAG, "%s stored: \"%s\"", key, value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        strlcpy(value, defaultVal, val_len);
    }
}

void _loadBoolValue(nvs_handle_t nvsHandle, const char* key, bool *value, bool defaultVal)
{
    esp_err_t err;
    int8_t output;
    err = nvs_get_i8(nvsHandle, key, &output);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *value = defaultVal;
    }
    else {
        *value = (output != 0);
    }
}
