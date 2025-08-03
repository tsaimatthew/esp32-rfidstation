// #include "bme280.h"


// Example code to read SPI register
    // uint8_t rxBuf[1] = {0};
    // spi_transaction_t bme280_trans = {
    //     .cmd = BME_ID | READ_MASK,
    //     .length = 0,
    //     .rxlength = 8,
    //     .rx_buffer = rxBuf
    // };
    // ESP_ERROR_CHECK(spi_device_transmit(bme280_handle, &bme280_trans));
    //print reg contents
    // printf("\n");
    // for (int i = 0; i < 1; i++) {
    //     printf("num %d: 0x%02X \n", i, rxBuf[i]);
    // }
    // printf("\n");