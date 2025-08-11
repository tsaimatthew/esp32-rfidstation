#include "ble.h"
#include "sensors.h"
#include "esp_nimble_hci.h"
#include <time.h>
#include "esp_system.h"
#include "esp_mac.h"
/* Global Variable Declarations */
static const char* LOG_TAG = "BLE";

char sensor_value[50] = "";
char rfid_value[50] = "";


//Notif vars
TaskHandle_t xHandle = NULL;
uint16_t conn_handle;
uint16_t notification_handle;
bool notify_state = true;

/*********
 * Define UUIDs
 *********/

//!! Service UUID: 0161844a-240c-4dff-b992-6300459a902c
static const ble_uuid128_t gatt_svr_svc_uuid =
    BLE_UUID128_INIT(0x2c, 0x90, 0x9a, 0x45, 0x00, 0x63, 0x92, 0xb9, 0xff, 0x4d, 0x0c, 0x24, 0x4a, 0x84, 0x61, 0x01);

//!! Characteristic UUID: 85453fa7-2516-47b1-8ab5-b309cc46ce2b
static const ble_uuid128_t gatt_svr_chr_mac_uuid =
    BLE_UUID128_INIT(0x2b, 0xce, 0x46, 0xcc, 0x09, 0xb3, 0xb5, 0x8a, 0xb1, 0x47, 0x16, 0x25, 0xa7, 0x3f, 0x45, 0x85);

//!! Characteristic UUID: ee4c2466-c729-4693-a087-d6082f04774a
static const ble_uuid128_t gatt_svr_chr_rfid_uuid =
    BLE_UUID128_INIT(0x4a, 0x77, 0x04, 0x2f, 0x08, 0xd6, 0x87, 0xa0, 0x93, 0x46, 0x29, 0xc7, 0x66, 0x24, 0x4c, 0xee);



/*********
 * gatt_svr_chr_access: defines callback
 *  Configures functions to run and data to return on BLE read
 *********/
int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc = 0;
    const ble_uuid_t *uuid = ctxt->chr->uuid;

    //Identify characteristic by UUID
    if (ble_uuid_cmp(uuid, &gatt_svr_chr_mac_uuid.u) == 0) //Generic Sensor
    {
        assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR); //can only read this attribute
        //respond with sensor value from components/sensors

        rc = os_mbuf_append(ctxt->om, &macAddr, sizeof macAddr);
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        
    } 
    else if (ble_uuid_cmp(uuid, &gatt_svr_chr_rfid_uuid.u) == 0) //RFID
    {
        assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR); //can only read this attribute
        char *rfid = read_rfid();
        rc = os_mbuf_append(ctxt->om, rfid, strlen(rfid));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    /* Unknown characteristic */
    assert (0);
    return BLE_ATT_ERR_UNLIKELY;
};

/*********
 * gatt_svr_svcs: defines GATT attribute table.
 *  All characteristics must be included in .characteristics
 *********/
const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                /* Characteristic: Sensor data */
                .uuid = &gatt_svr_chr_mac_uuid.u,
                .access_cb = gatt_svr_chr_access,
                .val_handle = &notification_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY 
            },
            {
                .uuid = &gatt_svr_chr_rfid_uuid.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY
            },
            {
                0
            }
        },
    },
    {
        0
    }
};

/*********
 * esp_bt_advertise: begins advertising with name configured in config
 *  Logs when advertisement set and start
 *********/
 void esp_bt_advertise(void)
{
    struct ble_hs_adv_fields fields;
    struct ble_gap_adv_params adv_params;
    int rc;
    
    /* Set advertisement data */
    memset(&fields, 0, sizeof fields);
    fields.name_is_complete = 1;
    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0)
    {
        ESP_LOGE(LOG_TAG, "Error setting advertisement data: rc = %d", rc);
        return;
    }

    /* Begin advertising */
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(BLE_GAP_DISC_MODE_GEN, NULL, BLE_HS_FOREVER, &adv_params, esp_bt_gap_event, NULL);
    if (rc != 0)
    {
        ESP_LOGE(LOG_TAG, "Error starting advertisement: rc = %d", rc);
        return;
    }
    ESP_LOGI(LOG_TAG, "Advertising started");
}

/************************************
 * esp_bt_gap_event: Controls connection and disconnect processes
 *  ESP32 stops advertising on connect and begins advertising on disconnect
 ************************************/
int esp_bt_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type)
    {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(LOG_TAG, "connection %s; status=%d ",
                       event->connect.status == 0 ? "established" : "failed",
                       event->connect.status);
            if (event->connect.status == 0)
            {
                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);
                esp_bt_print_conn_desc(&desc);
                conn_handle = event->connect.conn_handle; //store connection handle
            }
            if (event->connect.status != 0) {
                esp_bt_advertise(); //go back to advertising on disconnect
            }
            return 0;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(LOG_TAG, "disconnect; reason=%d", event->disconnect.reason);
            esp_bt_advertise();
            return 0;
        default:
            return 1;
    }
    return 2;
};

/************************************
 * Logs information about a connection
 ************************************/
 void esp_bt_print_conn_desc(struct ble_gap_conn_desc *desc)
{
    ESP_LOGI(LOG_TAG, "handle=%d our_ota_addr_type=%d our_ota_addr=",
                desc->conn_handle, desc->our_ota_addr.type);
    ESP_LOGI(LOG_TAG, " our_id_addr_type=%d our_id_addr=",
                desc->our_id_addr.type);
    ESP_LOGI(LOG_TAG, " peer_ota_addr_type=%d peer_ota_addr=",
                desc->peer_ota_addr.type);
    ESP_LOGI(LOG_TAG, " peer_id_addr_type=%d peer_id_addr=",
                desc->peer_id_addr.type);
    ESP_LOGI(LOG_TAG, " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                "encrypted=%d authenticated=%d bonded=%d\n",
                desc->conn_itvl, desc->conn_latency,
                desc->supervision_timeout,
                desc->sec_state.encrypted,
                desc->sec_state.authenticated,
                desc->sec_state.bonded);
};


/**************************
 * Configure BLE: Initialize NVS, HCI, and name
**************************/
int configure_ble() 
{
    int rc;
    /* BLE: Initialize HCI */
    if (nimble_port_init() != ESP_OK) return 1; //if NimBLE port init failed
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svr_svcs);
    ble_gatts_add_svcs(gatt_svr_svcs);

    /* BLE: Name Device */
    rc = ble_svc_gap_device_name_set(deviceName); //!! Set the name of this device
    assert(rc == 0);

    ESP_LOGI(LOG_TAG, "configured with rc=%d", rc);
    return rc;
};

/**************************
 * ble_app_on_sync: when configure_ble compeltes, this is called
**************************/
void ble_app_on_sync(void)
{
    esp_bt_advertise();  // safe to advertise now
    ESP_LOGI(LOG_TAG, "advertising");
};

/**************************
 * ble_host_task: RTOS task to manage BLE
 *  Pinned to core 0 by default
**************************/
void ble_host_task(void *param) {
    nimble_port_run(); // This function never returns
    nimble_port_freertos_deinit();
};

void vTaskSendNotification()
{
    struct os_mbuf *om;
    while (1)
    {
        if (notify_state)
        {
            /* TODO: DUMMY CODE */
            // rc = ble_gattc_notify_custom(conn_handle, notification_handle, om);

            uint8_t data[] = {read_sensor()};
            om = ble_hs_mbuf_from_flat(data, sizeof(data));
            ble_gatts_notify_custom(conn_handle, notification_handle, om);
            // notify_state = false;
        }
        
        /* Log Current Time */
        time_t now;
        char strftime_buf[64];
        struct tm timeinfo;
        // time(&now);
        // localtime_r(&now, &timeinfo);
        // strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        // ESP_LOGI("SNTP", "The current date/time in Los Angeles is: %s", strftime_buf);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    // Should never exit
    vTaskDelete(NULL);
}
