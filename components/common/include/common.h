/*******************
common.h: includes used across all components
*******************/

#ifndef COMMON_H
#define COMMON_H
/* Includes */
/* STD APIs */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* ESP APIs */
#include "esp_log.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

/* FreeRTOS APIs */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* Project Global Variables and Defines*/
#define WIFI_SSID_MAX 32
#define WIFI_PSK_MAX 64
#define MAX_DEVICE_NAME_LENGTH 28
#define MAX_MQTT_ENDPT_LENGTH 64
#define MAX_CERT_SIZE 4096

extern char deviceName[MAX_DEVICE_NAME_LENGTH + 1];
extern char ssid[WIFI_SSID_MAX + 1];
extern char psk[WIFI_PSK_MAX + 1];
extern char mqttEndpt[MAX_MQTT_ENDPT_LENGTH + 1];
extern char tz[29];
extern char macAddr[18];

extern char client_key_pem[MAX_CERT_SIZE];
extern char client_cert_pem[MAX_CERT_SIZE];
extern char server_cert_pem[MAX_CERT_SIZE];

extern bool wifiEnable;
extern bool rfidEnable;
extern bool tempEnable;
extern bool pressureEnable;
extern bool humidityEnable;
extern int16_t sampleRate;

/* Interrupt Flag*/
extern bool notify_state;
extern TaskHandle_t xHandle;
#endif //COMMON_H