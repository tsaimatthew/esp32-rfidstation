/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
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


    // client_cert_pem = "-----BEGIN CERTIFICATE-----\nMIICuzCCAaOgAwIBAgIUGUyYGR6UEsi9Jrhw9bXa50CSS2MwDQYJKoZIhvcNAQELBQAwGDEWMBQGA1UEAwwNSFdBUFAgUm9vdCBDQTAeFw0yNTA3MjQwMDQ1MTlaFw0yNjA3MjQwMDQ1MTlaMBcxFTATBgNVBAMMDEFDMTUxOEQ0QTRGMDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKjHN9EhQpn6J6qu2IUcZD1jsZUDr9aXKprvfAAQLqXq+eJNHNt43vLc+ExtKM/zSXTl7Rr+dD29vEkquwPsnKnctq43tc8PJh99hfZzOKPfjKAIkBcECTsUrCu2OKCWBX2kHvKfTOazEvrCTSapQS3ZaX6DRVDqPGzRlDNK6Zb2Nwj+jtJftKv2Ui9X1fITYsnEMD6ReDZWpbLYLw0Lnw7/o4Bkheh1kjQsop14DgMKvGZ7XKWf5HvSRKlatLHuF7RtuHq33r833SNhQSpuyRRHSItvnJu8iFbfV7jNxnjwQY72jhiAwqB7zsFzioX4JWhKWv1doYHxKcTILeRwMDUCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAqaOe/FaRNdCm2+f3if+e3o1iayjvu/SlPAdCgS55I2IJxsFWNnyaZdWjoTvNMTpHf6HQQphxbXCM4Hb3fkGya4GK/7Pbk+IakG7GmyjBA/wTKixOWwyinE84U5Uaq8tZ1vmYR5/7NL1/jzDXNvCnK6HzSP0Meq4HAd3qhnZhPrUrYaqQOPcXK88LkIgvAmCFCSTvpBMvIDiAVmalo+1xge+jjZMCAXmEKH5gwxsURTPTFwNJXUNpqw00uR9i/JyWLAn0NqFvQas8SEhtiaWl4QHN6Y8xuGbAUhYlEoIrgKFyFeFZ2MrlturSd0oeGd8NaqXDTbdmtvvYTKmTGaFPSQ==\n-----END CERTIFICATE-----\n";
    
    // client_key_pem = "-----BEGIN RSA PRIVATE KEY-----\nMIIEowIBAAKCAQEAqMc30SFCmfonqq7YhRxkPWOxlQOv1pcqmu98ABAuper54k0c23je8tz4TG0oz/NJdOXtGv50Pb28SSq7A+ycqdy2rje1zw8mH32F9nM4o9+MoAiQFwQJOxSsK7Y4oJYFfaQe8p9M5rMS+sJNJqlBLdlpfoNFUOo8bNGUM0rplvY3CP6O0l+0q/ZSL1fV8hNiycQwPpF4NlalstgvDQufDv+jgGSF6HWSNCyinXgOAwq8ZntcpZ/ke9JEqVq0se4XtG24erfevzfdI2FBKm7JFEdIi2+cm7yIVt9XuM3GePBBjvaOGIDCoHvOwXOKhfglaEpa/V2hgfEpxMgt5HAwNQIDAQABAoIBABLKGuytRzzdHI1j6bbn8kDjWGG+h1Tcf2HAR426P3c5MZh//TZxvmBLOVlIzcJY01SwRDU9HrPA67U5jJhjPw2qBKxgh10F0riuwLsvGJ8lxAIM8f2d9WkeZAx5vNQj33idTNS151nHldVUzEIBlcAE9DmhY5YefZufuV/8dwTXNf5bcyTqnETLAnyUHEuco1eaXjkwCdDZ703mr+BB+zL8iwPeYrRJeIV0uVQTaMqFwKgG8kyzyLly2D5yZzkD6s/tbmw3Kzg4/OsDbXtUVf4EENP6r/Duzhm2vCSUR6rxH5qMkGXTVtcDmev8UxxfSuvC4VJacQnaiZy62SUhxgECgYEA1FywZS2++cvaeAH/07J3P8Y4wPi1u1HobkZjtJGiy1J9OQPR1ozsRm45Tnapj+eqz+NrXcJp0GetGtMGL2WbQ2PASElBve9JZS5dJ0EeHaOVd12ZZ/jkVwykklO0s8itjTd893uY4041TwC4vYDVmIa4QMAJTu+d5IGIlLvY1t8CgYEAy3XMbP+nbNx8KIHiXTShOeHw2CFtmO5neWTyT5JcorhO1kL64Bsb7y3oJK9XlcIpowvHNlalm3lab/ekCoLzQadn4dHTnyY9cjPhjroX6+tJ/35GWUfw7Z9kq7wZxB6McJBPqQ4tmneF56CCsTTFGfeK6BdNd3k3o+KH0C1Cv2sCgYEAmrxjB4ZatlcUfAcw6ocKnxyHNAzFFpWrH9cIRYUssqwLdGTVHFkYvIUKGqMaEDJE4y+Zhvrm8STqHDaRNi36AROJAuLmFUhrGV+8HqMzF3OfsBcydXEEqG0c6UY20B00YEaDNy0HDPFqpIMpGWPrvzTCwuNqpOqyCNNduspZSM8CgYByxg795fIinPaFO0/g2FGi/2wH4EOdI8/HUUTH0n7jZClFAR/Y0DIf6LuuBiPYXWFzkq9cXeCqJfj4dLBbJafn/3HAl8dTXhUHmXDCPQRFl8N0l11D/CtTQLlfj4rRcZIz5ZSqf08GdipZkdhn8qbFkTkQ2CRhI0ZZ8u4+Z2nh5QKBgBdqZiSKIkP5RwFaZs+pt/o+dV3gfWAqIUgxidGVt3+BdX48Y5UJ39wpusXXOEYeohjdupEJeLSsva2jadxkos8luHtQvZQ/FfEURLhY1cBp9vV7npvIsyJ84ldaY4fueqaltYZJd77ziJ0MpU4K8g/7kJG9x53L3uyxClg0V7IA\n-----END RSA PRIVATE KEY-----\n";
    // server_cert_pem = "-----BEGIN CERTIFICATE-----\nMIIC0TCCAbmgAwIBAgIUDpK8NswmdYCbh9E/CGmaQ8QKn4kwDQYJKoZIhvcNAQELBQAwGDEWMBQGA1UEAwwNSFdBUFAgUm9vdCBDQTAeFw0yNTA3MjEwMjE1MTZaFw0zNTA3MTkwMjE1MTZaMBgxFjAUBgNVBAMMDUhXQVBQIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCpt5q3ASWVwMz0Oq/AwlF9hGQkl3h8hPkCZIHKpARRljuJE7NVjrWhvTLpmJL7SZoKS+9RvhdgqTnQ8vSWI1RXDqSoig2OlQww1ikGR3FT721rI1K5IpHFIQNCEgf06EhJn52oT5xImzbr/3xqqfUb+BsjiSvU+YFkRM0c+2gQuO6EDLsxpjLSJyu+xP+qrvTpd+LBBr0+h0mxS2LUnMh0+8LEiYp+OXlPx+Cj5KCoYNvztZwRVfg2ujVyeyAHCDOqdA6bA4rmYqegX8I6pRXa9FSiJs230nZOJU0qXGJfahJTUmzrk/RCC3Y2NUe/2FLRzHEQMy9+2DZ3pDN6GjCVAgMBAAGjEzARMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEBAIPl2/SuIXBWlXE95n584w6R1uzv28NpGpl85zjjnYLw/uGKbF6GfLxpr6yepgskogCAL4uFQ5YlMjfaXUBnXqxQpYOIviInihSTO3KN8wNgFGUqGJLN4YSTMOJ9RRmPh8Fzc+XIR4CO96AyfL75XSu7czGksEaWvsu7k2j5xQugEzBF+VGRYzrABfoGoUrh5gai2bCbZJOJ00LMvV7EtWp/6n2qUgc10+x5d/VFM3DKy5SmllV8/Kp9qm4gev2y2GvP6wmo78XXk3KTbNuh/43KuDjVZCsCEjRqt8wRvn3aYCOCBwZrHMsfxcUuugZiHfVsc+oRsoRgqfJyxhwy/oM=\n-----END CERTIFICATE-----\n";

    // nvs_set_str(nvsHandle, "client_cert", client_cert_pem);
    // nvs_set_str(nvsHandle, "client_key", client_key_pem);
    // nvs_set_str(nvsHandle, "server_cert", server_cert_pem);
    // nvs_set_str(nvsHandle, "mqttEndpt", "mqtts://mqtt.matthewtsai.uk:1883");
    
    /* Read Values */
    _loadStrValue(nvsHandle, "client_cert", client_cert_pem, sizeof(client_cert_pem), "");
    _loadStrValue(nvsHandle, "client_key", client_key_pem, sizeof(client_key_pem), "");
    _loadStrValue(nvsHandle, "server_cert", server_cert_pem, sizeof(server_cert_pem), "");

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
