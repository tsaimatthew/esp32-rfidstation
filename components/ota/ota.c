/* Includes */
#include "esp_wifi.h"
#include "common.h"
#include "esp_system.h"
#include "ota.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"
#include <esp_netif_sntp.h>

/* Defines */
#define TAG "esp_wifi"
#define WIFI_CONNECTED_BIT BIT0

/* Global Variables */
int retry_num = 0;
EventGroupHandle_t wifiEventGroup = NULL;

/* WiFi Functions */
#ifdef CONFIG_WIFI_ENABLE

esp_err_t validate_image_header(esp_app_desc_t *newAppInfo);

void wifi_config()
{
    /* Initialize network stack */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    /* Initialize WiFi and pin to core 0*/
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.wifi_task_core_id = 0;
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    /* Add callbacks for WiFi events and on obtaining IP */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id
                                                    ));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip
                                                    ));
    /* Init WiFi Connect Flags */
    if (wifiEventGroup == NULL) {
        wifiEventGroup = xEventGroupCreate();
        assert(wifiEventGroup);
    }

    /* Connect to WiFi */
    const char *default_ssid = CONFIG_ESP_WIFI_SSID;
    const char *default_password = CONFIG_ESP_WIFI_PASSWORD;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    wifiLogin(default_ssid, default_password);
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Configure SNTP */
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);
    esp_netif_sntp_start();
}

/* Logging and retry for WiFi task*/
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // connect to wifi on start
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) esp_wifi_connect();

    //retry connections if connection fails
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
        retry_num++;
        ESP_LOGI(TAG, "retrying connection to AP, attempt #%d", retry_num);

    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
        retry_num = 0;
    }
    else xEventGroupClearBits(wifiEventGroup, WIFI_CONNECTED_BIT); //clear wifi connect bits as catch all
}

void wifiLogin(const char *ssid, const char *password)
{

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK, //WPA2 by default
        },
    };
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';

    /* Set config */
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
}

void wifi_connect_task(void *pvParameters)
{
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect: %s", esp_err_to_name(err));
    }
    vTaskDelete(NULL);
}

/* OTA Tools */

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}
/* Check if current firmware is up to date */
esp_err_t validate_image_header(esp_app_desc_t *newAppInfo)
{
    if (newAppInfo == NULL) return ESP_ERR_INVALID_ARG;
    const esp_partition_t *curPart = esp_ota_get_running_partition();
    esp_app_desc_t runningAppInfo;
    if (esp_ota_get_partition_description(curPart, &runningAppInfo) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", runningAppInfo.version);
    }
    if (memcpy(newAppInfo->version, runningAppInfo.version, sizeof(newAppInfo->version)) == 0)
    {
        ESP_LOGW(TAG, "No newer updates available");
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* Update if newer version is available */
void ota_check()
{
    esp_err_t err;
    esp_err_t ota_finish_err = ESP_OK;

    esp_http_client_config_t config = {
        .url = CONFIG_FIRMWARE_UPGRADE_URL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = _http_event_handler,
        .keep_alive_enable = true,
        .timeout_ms = 30 * 1000,
        .buffer_size     = 2048,
        .buffer_size_tx  = 1024,
    };
    esp_https_ota_config_t ota_config = {
        .http_config = &config,
        .partial_http_download = true,
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA Begin Failed");
        vTaskDelete(NULL);
    }

    esp_app_desc_t app_desc = {};
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA Get App Desc Failed");
        vTaskDelete(NULL);
        esp_https_ota_abort(https_ota_handle);
    }

    err = validate_image_header(&app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA Image Validation Failed");
        vTaskDelete(NULL);
        esp_https_ota_abort(https_ota_handle);       
    }

    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) break;
        const size_t len = esp_https_ota_get_image_len_read(https_ota_handle);
        ESP_LOGD(TAG, "Image bytes read: %d", len);    
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) ESP_LOGE(TAG, "Complete data not received.");
    else {
        ota_finish_err = esp_https_ota_finish(https_ota_handle);
        if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
            ESP_LOGI(TAG, "OTA Upgrade Successful. Rebooting now. ");
            vTaskDelay(1000/portTICK_PERIOD_MS);
            esp_restart();
        } else {
            ESP_LOGE(TAG, "OTA FAILED 0x%x", ota_finish_err);
            vTaskDelete(NULL);
        }
    }
}
void ota_task(void *arg)
{
    #ifdef CONFIG_DEBUG
    const TickType_t delay = pdMS_TO_TICKS(60*1000); //one minute
    #else
    const TickType_t delay = pdMS_TO_TICKS(24*60*60*1000); //one day
    #endif 
    vTaskDelay(pdMS_TO_TICKS(30*1000));
    while (true)
    {
        if (xEventGroupGetBits(wifiEventGroup) & WIFI_CONNECTED_BIT) ota_check();
        else {
            esp_wifi_stop();
            esp_wifi_start();
        }
        vTaskDelay(delay);
    }
}

#endif //WIFI_ENABLE