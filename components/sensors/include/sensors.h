/*******************
ble.h: providing API to communicate with BLE 
*******************/

#ifndef SENSORS_H
#define SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif
#include "mqtt_client.h"

int read_sensor();
char *read_rfid();
void run_mqtt();
void send_mqtt(esp_mqtt_client_handle_t client, char topic[], char *message);
esp_err_t initSpi();
void vTaskReadBME280();
int bme280_compensate_T(uint32_t uncomp_T);

#ifdef __cplusplus
}
#endif

#endif