#include "bme280.h"
#include "spi.h"

#define TAG "BME280"

uint16_t dig_T1_val =   0;
int16_t dig_T2_val =    0;
int16_t dig_T3_val =    0;
uint16_t dig_P1_val =   0;
int16_t dig_P2_val =    0;
int16_t dig_P3_val =    0;
int16_t dig_P4_val =    0;
int16_t dig_P5_val =    0;
int16_t dig_P6_val =    0;
int16_t dig_P7_val =    0;
int16_t dig_P8_val =    0;
int16_t dig_P9_val =    0;
uint8_t dig_H1_val=     0;
int16_t dig_H2_val =    0;
uint8_t dig_H3_val=     0;
int16_t dig_H4_val     =0;
int16_t dig_H5_val =    0;
int8_t  dig_H6_val =    0;

int t_fine = 0;
uint8_t txBuf[1] = {BME_CONFIG};
uint8_t ctrl_meas = BME_CONFIG;
uint8_t ctrl_hum = 0x01;
uint8_t tempBuf[3] = {0};
uint8_t status[1] = {0};
uint8_t regbuf[8] = {0};

void initBME280(spi_device_handle_t *spiHandle)
{
    /* LOAD correction params into memory and confirm device ID */
    //Confirm device ID
    uint8_t id[1];
    readSpi(0xD0, id, spiHandle, 8);
    if (id[0] != 0x60) ESP_LOGI(TAG, "BME280 ID does not match expected. Is: 0x%02X instead of 0x60", id[0]);

    /* LOAD T1 to T3 */
    uint8_t buf[2] = {0};
    readSpi(DIG_T1, buf, spiHandle, 16);
    dig_T1_val = (((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array

    readSpi(DIG_T2, buf, spiHandle, 16);
    dig_T2_val = (int16_t)(((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array

    readSpi(DIG_T3, buf, spiHandle, 16);
    dig_T3_val = (int16_t)(((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array

    /* LOAD P1 through P9 */
    readSpi(DIG_P1, buf, spiHandle, 16);
    dig_P1_val = (((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array
    
    readSpi(DIG_P2, buf, spiHandle, 16);
    dig_P2_val = (int16_t)(((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array

    readSpi(DIG_P3, buf, spiHandle, 16);
    dig_P3_val = (int16_t)(((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array

    readSpi(DIG_P4, buf, spiHandle, 16);
    dig_P4_val = (int16_t)(((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array

    readSpi(DIG_P5, buf, spiHandle, 16);
    dig_P5_val = (int16_t)(((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array

    readSpi(DIG_P6, buf, spiHandle, 16);
    dig_P6_val = (int16_t)(((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array

    readSpi(DIG_P7, buf, spiHandle, 16);
    dig_P7_val = (int16_t)(((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array

    readSpi(DIG_P8, buf, spiHandle, 16);
    dig_P8_val = (int16_t)(((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array

    readSpi(DIG_P9, buf, spiHandle, 16);
    dig_P9_val = (int16_t)(((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array

    /* LOAD H1 through H6 */
    readSpi(DIG_H1, buf, spiHandle, 8);
    dig_H1_val = (unsigned char)buf[0];
    memset(buf, 0, sizeof(buf)); //reset array

    readSpi(DIG_H2, buf, spiHandle, 16);
    dig_H2_val = (int16_t)(((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array

    readSpi(DIG_H3, buf, spiHandle, 8);
    dig_H3_val = (unsigned char)buf[0];
    memset(buf, 0, sizeof(buf)); //reset array

    uint8_t e4 = 0, e5 = 0, e6 = 0;
    readSpi(DIG_H4, &e4, spiHandle, 8);
    readSpi(0xE5, &e5, spiHandle, 8);
    readSpi(0xE6, &e6, spiHandle, 8);

    dig_H4_val = (int16_t)(((int16_t)e4<<4) | ((int16_t)e5 & 0x0F));
    dig_H5_val = (int16_t)(((int16_t)e6 << 4) | ((int16_t)e5 >> 4));

    readSpi(DIG_H6, buf, spiHandle, 8);
    dig_H6_val = (int8_t)buf[0];
}

void readBME280(spi_device_handle_t *spiHandle, int *temp32, uint32_t *pressure32, double *humidity32)
{
    writeSpi(CTRL_HUM, &ctrl_hum, spiHandle, 8); //init forced mode humidity
    writeSpi(CTRL_MEAS, &ctrl_meas, spiHandle, 8); //init forced mode temp/pressure
    memset(regbuf, 0, sizeof(regbuf));
    memset(status, 0, sizeof(status)); //reset array
    do {
        // Wait for reading to complete
        readSpi(BME_STATUS, status, spiHandle, 8);
    } while (status[0] & 0x08);

    //burst read registers
    readSpi(PRESS_MSB, regbuf, spiHandle, 8*8);

    //Process temperature
    *temp32 = 0;
    *temp32 |= ((int32_t)regbuf[3] << 12);
    *temp32 |= ((int32_t)regbuf[4] << 4);
    *temp32 |= ((int32_t)regbuf[5] >> 4);
    *temp32 = bme280_compensate_T(*temp32);

    //Process Pressure
    *pressure32 = 0;
    *pressure32 |= ((int32_t)regbuf[0] << 12);
    *pressure32 |= ((int32_t)regbuf[1] << 4);
    *pressure32 |= ((int32_t)regbuf[2] >> 4);
    *pressure32 = bme280_compensate_P(*pressure32);

    //Process Humidity
    uint16_t raw_humidity = 0;
    raw_humidity |= (regbuf[6] << 8);
    raw_humidity |= (regbuf[7]);
    *humidity32 = bme280_compensate_H((int32_t)raw_humidity);
}

int bme280_compensate_T(uint32_t uncomp_T)
{
    int var1, var2, T;
    var1 = ((((uncomp_T>>3) - ((int32_t)dig_T1_val<<1))) * ((int32_t)dig_T2_val)) >> 11;
    var2 = (((((uncomp_T>>4) - ((int32_t)dig_T1_val)) * ((uncomp_T>>4) - ((int32_t)dig_T1_val)))>>12) *
            ((int32_t)dig_T3_val)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

uint32_t bme280_compensate_P(int uncomp_P)
{
    long long int var1, var2, p;
    var1 = ((long long int)t_fine) - 128000;
    var2 = var1 * var1 * (long long int)dig_P6_val;
    var2 += ((var1 * (long long int)dig_P5_val)<<17);
    var2 += (((long long int)dig_P4_val)<<35);
    var1 = ((var1*var1*(long long int)dig_P3_val)>>8) + ((var1* (long long int)dig_P2_val)<<12);
    var1 = (((((long long int)1)<<47)+var1))*((long long int)dig_P1_val) >> 33;
    if (var1 == 0) return 0;
    p = 1048576 - uncomp_P;
    p = (((p<<31)-var2)*3125) / var1;
    var1 = (((long long int)dig_P9_val) * (p>>13) * (p>>13)) >> 25;
    var2 = (((long long int)dig_P8_val) * p) >> 19;
    p = ((p+var1+var2) >> 8) + (((long long int)dig_P7_val)<<4);
    return (uint32_t)p;
}

double bme280_compensate_H(int32_t adc_H)
{
    double var_H;
    var_H = (((double)t_fine) - 76800.0);
    var_H = (adc_H - (((double)dig_H4_val) * 64.0 + ((double)dig_H5_val) / 16384.0 *
            var_H)) * (((double)dig_H2_val) / 65536.0 * (1.0 + ((double)dig_H6_val) /
            67108864.0 * var_H *
            (1.0 + ((double)dig_H3_val) / 67108864.0 * var_H)));
    var_H = var_H * (1.0 - ((double)dig_H1_val) * var_H / 524288.0);
    if (var_H > 100.0) var_H = 100.0;
    else if (var_H < 0.0) var_H = 0.0;
    return var_H;
}