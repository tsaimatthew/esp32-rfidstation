#ifndef SPI_H
#define SPI_H
#include "common.h"
#include "driver/spi_master.h"
void readSpi(uint8_t reg, uint8_t *rxBuf, spi_device_handle_t *spiHandle, int rxLength);
void writeSpi(uint8_t reg, uint8_t *txBuf, spi_device_handle_t *spiHandle, int txLength);
void duplexSpi(uint8_t *txBuf, uint8_t *rxBuf, spi_device_handle_t *spiHandle, int txLength, int rxLength);

#endif