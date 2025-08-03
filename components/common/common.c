#include "common.h"

/* RUNTIME GLOBAL VARIABLES: Users can change during runtime */
char deviceName[MAX_DEVICE_NAME_LENGTH + 1] = {0};
char ssid[WIFI_SSID_MAX + 1] = {0};
char psk[WIFI_PSK_MAX + 1] = {0};
char mqttEndpt[MAX_MQTT_ENDPT_LENGTH + 1] = {0};
char tz[29] = {0};
char macAddr[18] = {0};
char *client_key_pem = "";
char *client_cert_pem = "";
char *server_cert_pem = "";
bool wifiEnable = true;
bool rfidEnable = false;
bool tempEnable = false;
bool pressureEnable = false;
bool humidityEnable = false;

/* FIXED GLOBAL VARIABLES: Must be set during compile time. 
    Users should not expect to change these */
int16_t sampleRate = 10; //Hz, frequency of sample readings for every sensor