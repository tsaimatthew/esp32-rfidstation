#ifndef BME280_H
#define BME280_H

#include "driver/spi_master.h"
#include "common.h"

#define HUM_LSB    0xFE
#define HUM_MSB    0xFD
#define TEMP_XLSB  0xFC
#define TEMP_LSB   0xFB
#define TEMP_MSB   0xFA
#define PRESS_XLSB 0xF9
#define PRESS_LSB  0xF8
#define PRESS_MSB  0xF7
#define CTRL_HUM   0XF2
#define READ_MASK  0x80 //1000 0000
#define WRITE_MASK 0x7F //0111 1111
#define BME_ID     0xD0
#define CTRL_MEAS  0xF4
#define BME_CONFIG 0x25 //0010 0101
#define BME_STATUS 0xF3

#define DIG_T1     0x88
#define DIG_T2     0x8A
#define DIG_T3     0x8C

#define DIG_P1     0X8E
#define DIG_P2     0X90
#define DIG_P3     0X92
#define DIG_P4     0X94
#define DIG_P5     0X96
#define DIG_P6     0X98
#define DIG_P7     0X9A
#define DIG_P8     0X9C
#define DIG_P9     0X9E

#define DIG_H1     0xA1
#define DIG_H2     0xE1
#define DIG_H3     0xE3
#define DIG_H4     0xE4
#define DIG_H5     0xE5
#define DIG_H6     0xE7

void initBME280(spi_device_handle_t *spiHandle);

void readBME280(spi_device_handle_t *spiHandle, int *temp32, uint32_t *pressure32, double *humidity32);

int readTemperature(spi_device_handle_t *spiHandle);
uint32_t readPressure(spi_device_handle_t *spiHandle);
uint32_t readHumidity(spi_device_handle_t *spiHandle);

int bme280_compensate_T(uint32_t uncomp_T);
uint32_t bme280_compensate_P(int uncomp_P);
double bme280_compensate_H(int32_t adc_H);
#endif