#ifndef BME280_H
#define BME280_H

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
#endif