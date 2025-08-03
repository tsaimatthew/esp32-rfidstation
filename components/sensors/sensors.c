//Includes
#include "sensors.h"
#include "common.h"
#include "string.h"
#include "hal/spi_types.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "bme280.h"
#include "spi.h"

//Defines
#define TAG "esp_sensors"


//Global Variables
int retValDummy = 0;
bool mqttConnected = false;
static esp_mqtt_client_handle_t mqtt_client = NULL;
static spi_device_handle_t bme280_handle;
TaskHandle_t bmeHandle = NULL;
uint16_t dig_T1_val = 0;
int16_t dig_T2_val = 0;
int16_t dig_T3_val = 0;

/* SPI Handler Functions */
int read_sensor() 
{
    return retValDummy++;
}

char *read_rfid()
{
    char *retVal = "6b00db41-b6f2-4aa4-bb4f-ad73b3804fe3";
    return retVal;
}

esp_err_t initSpi()
{ 
    /******************
    Set up SPI and attach devices, set/read config params
    ******************/
    vTaskDelay(pdMS_TO_TICKS(10)); // 10 ms delay before SPI comms
    const spi_bus_config_t config = {
        .mosi_io_num = CONFIG_MOSI_PIN,
        .quadhd_io_num = -1, //quad/octal not used
        .miso_io_num = CONFIG_MISO_PIN,
        .quadwp_io_num = -1,
        .sclk_io_num = CONFIG_SCK_PIN,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_1,
    };
    esp_err_t err = spi_bus_initialize(SPI2_HOST, &config, SPI_DMA_DISABLED);
    if (err != ESP_OK) return err;
    //Add BME280
    const spi_device_interface_config_t bme280_config = {
        .command_bits = 0,
        .address_bits = 8,
        .dummy_bits = 0,
        .mode = 0, //BME280 supports 00 and 11 mode
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .clock_speed_hz = 2000000,
        .input_delay_ns = 0,
        .spics_io_num = CONFIG_BME280_CS_PIN,
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &bme280_config, &bme280_handle));
    // uint8_t txBuf[1] = {BME_CONFIG};
    // spi_transaction_t bme280_trans = {
    //     .cmd = CTRL_MEAS & WRITE_MASK,
    //     .length = 8,
    //     .rxlength = 0,
    //     .rx_buffer = NULL,
    //     .tx_buffer = txBuf
    // };
    // ESP_ERROR_CHECK(spi_device_transmit(bme280_handle, &bme280_trans));

    //Confirm device ID
    uint8_t id[1];
    readSpi(0xD0, id, &bme280_handle, 8);
    printf("0x%02X \n", id[0]);

    //Load t1, t2, t3 into memory
    uint8_t buf[2] = {0};
    readSpi(DIG_T1, buf, &bme280_handle, 16);
    dig_T1_val = (((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array

    readSpi(DIG_T2, buf, &bme280_handle, 16);
    dig_T2_val = (int16_t)(((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array

    readSpi(DIG_T3, buf, &bme280_handle, 16);
    dig_T3_val = (int16_t)(((uint16_t)buf[1]) << 8) | ((uint16_t)buf[0]);
    memset(buf, 0, sizeof(buf)); //reset array
    return ESP_OK;
};

void vTaskReadBME280()
{
    uint8_t txBuf[1] = {BME_CONFIG};
    uint8_t tempBuf[3] = {0};
    uint8_t status[1] = {0};
    uint8_t tempReg[4] = {TEMP_MSB|READ_MASK, 0, 0, 0};
    while (1)
    {
        printf("BMECONFIG: 0x%02X \n", txBuf[0]);
        // uint8_t tempBuf[3] = {0};
        // spi_transaction_t transaction = {
        //     .cmd = TEMP_MSB & READ_MASK,
        //     .length = 0,
        //     .rxlength = 24,
        //     .rx_buffer = tempBuf,
        // };
        // ESP_ERROR_CHECK(spi_device_transmit(bme280_handle, &transaction));
        /* Read Temperature */
        
        writeSpi(CTRL_MEAS, txBuf, &bme280_handle, 8);
        memset(tempBuf, 0, sizeof(tempBuf)); //reset array
        memset(status, 0, sizeof(status)); //reset array
        uint8_t ctrl_meas[1];
        do {
            readSpi(BME_STATUS, status, &bme280_handle, 8);
        } while (status[0] & 0x08);

        printf("Status: 0x%02X \n", status[0]);

        readSpi(CTRL_MEAS, ctrl_meas, &bme280_handle, 8);
        printf("0x%02X \n", ctrl_meas[0]);
        vTaskDelay(pdMS_TO_TICKS(20));

        duplexSpi()

        readSpi(TEMP_MSB, tempBuf, &bme280_handle, 24);
        // print reg contents
        int32_t temp32 = 0;
        printf("\n");
        printf("MSB: 0x%02X  LSB: 0x%02X  XLSB: 0x%02X\n",
            tempBuf[0], tempBuf[1], tempBuf[2]);
        temp32 |= ((int32_t)tempBuf[0] << 12);
        temp32 |= ((int32_t)tempBuf[1] << 4);
        temp32 |= ((int32_t)tempBuf[2] >> 4);


        printf("Final Temp: %ld\n", temp32);
        printf("compensated temp: %d", bme280_compensate_T(temp32));
        printf("\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int bme280_compensate_T(uint32_t uncomp_T)
{
    int var1, var2, T;
    var1 = ((((uncomp_T>>3) - ((int32_t)dig_T1_val<<1))) * ((int32_t)dig_T2_val)) >> 11;
    var2 = (((((uncomp_T>>4) - ((int32_t)dig_T1_val)) * ((uncomp_T>>4) - ((int32_t)dig_T1_val)))>>12) *
            ((int32_t)dig_T3_val)) >> 14;
    // var1 = ((((uncomp_T >> 3) - ((int32_t)dig_T1_val << 1))) * ((int32_t)dig_T2_val)) >> 11;
    // var2 = (((((uncomp_T >> 4) - ((int32_t)dig_T1_val)) *
    //         ((uncomp_T >> 4) - ((int32_t)dig_T1_val))) >> 12) *
    //         ((int32_t)dig_T3_val)) >> 14;
    int t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

/* MQTT Handler Functions */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        mqttConnected = true;
        send_mqtt(client, "/test",  "world");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        mqttConnected = false;
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

esp_mqtt_client_handle_t start_mqtt(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = mqttEndpt,
            .verification.certificate = (const char *)server_cert_pem,
            .verification.skip_cert_common_name_check = true,
        },
        .credentials = {
            .authentication = {
                .certificate = (const char *)client_cert_pem,
                .key = (const char *)client_key_pem,
            }
        },
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    return mqtt_client;
}

void send_mqtt(esp_mqtt_client_handle_t client, char topic[], char *message)
{
    char mac[50] = {0};
    strcpy(mac, macAddr);
    topic = "/test";
    strcat(mac, topic);
    printf(mac);
    esp_mqtt_client_publish(client, mac, message, 0, 2, 0);
}

void run_mqtt()
{
    if (mqtt_client == NULL) mqtt_client = start_mqtt();
    else esp_mqtt_client_reconnect(mqtt_client);
}