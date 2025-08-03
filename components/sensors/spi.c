#include "common.h"
#include "spi.h"
#include "driver/spi_common.h"
#include "hal/spi_types.h"
#include "bme280.h"

void readSpi(uint8_t reg, uint8_t *rxBuf, spi_device_handle_t *spiHandle, int rxLength)
{
    /****************
     * readSpi at given address
     * input: register
    ****************/
    spi_transaction_t transaction = {
        .cmd = 0,
        .addr = reg | READ_MASK,
        // .length = 8 + rxLength,
        .rxlength = rxLength,
        .rx_buffer = rxBuf,
        .length = 0,
    };
    ESP_ERROR_CHECK(spi_device_transmit(*spiHandle, &transaction));
}
void writeSpi(uint8_t reg, uint8_t *txBuf, spi_device_handle_t *spiHandle, int txLength)
{
    spi_transaction_t transaction = {
        .cmd = 0,
        .addr = reg & WRITE_MASK,
        .length = txLength,
        .rx_buffer = NULL,
        .tx_buffer = txBuf
    };
    ESP_ERROR_CHECK(spi_device_transmit(*spiHandle, &transaction));
}

void duplexSpi(uint8_t *txBuf, uint8_t *rxBuf, spi_device_handle_t *spiHandle, int txLength, int rxLength)
{
    spi_transaction_t transaction = {
        .length = rxLength,
        .rxlength = rxLength,
        .rx_buffer = rxBuf,
        .tx_buffer = txBuf
    };
    ESP_ERROR_CHECK(spi_device_transmit(*spiHandle, &transaction));
}