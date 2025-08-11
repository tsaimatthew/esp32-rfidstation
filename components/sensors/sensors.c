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
    initBME280(&bme280_handle);
    return ESP_OK;
};

void vTaskReadBME280()
{
    int temp32 = 0;
    uint32_t pressure32 = 0;
    double humidity32 = 0;
    while (1)
    {
        readBME280(&bme280_handle, &temp32, &pressure32, &humidity32);
        if (mqtt_client)
        {
            char buf[12];
            //Send temperature
            sprintf(buf, "%d", temp32);
            send_mqtt(mqtt_client, "/temp", buf);
            memset(buf, 0, sizeof(buf));

            //Send Pressure
            sprintf(buf, "%lu", pressure32);
            send_mqtt(mqtt_client, "/pressure", buf);
            memset(buf, 0, sizeof(buf));

            //Send Humidity
            sprintf(buf, "%f", humidity32);
            send_mqtt(mqtt_client, "/humidity", buf);
        }
        vTaskDelay(60000 / portTICK_PERIOD_MS); //5m delay
    }
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
        subscribe_mqtt(client, "/otaEnable", 2);
        subscribe_mqtt(client, "/bleEnable", 2);
        subscribe_mqtt(client, "/bme280SampleRateM", 2);

        mqttConnected = true;
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
        .session = {
            .keepalive = 60
        }
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    return mqtt_client;
}

void send_mqtt(esp_mqtt_client_handle_t client, char *topic, char *message)
{
    char mac[50] = {0};
    strcpy(mac, macAddr);
    strcat(mac, topic);
    ESP_LOGI(TAG, "Topic: %s", mac);
    esp_mqtt_client_publish(client, mac, message, 0, 0, 0);
}

void subscribe_mqtt(esp_mqtt_client_handle_t client, char *subtopic, int qos)
{
    char topic[50] = {0};
    strcpy(topic, macAddr);
    strcat(topic, subtopic);
    esp_mqtt_client_subscribe(client, topic, qos);
    ESP_LOGI(TAG, "Subscribed to %s", topic);
}

void run_mqtt()
{
    if (mqtt_client == NULL) mqtt_client = start_mqtt();
    else esp_mqtt_client_reconnect(mqtt_client);
}