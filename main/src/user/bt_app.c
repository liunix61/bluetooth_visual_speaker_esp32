/*
 * bt_app.c
 *
 *  Created on: 2018-03-09 13:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "core/os.h"
#include "core/app.h"

#include "user/bt_av.h"
#include "user/bt_app.h"

#define BT_APP_TAG "bt_app"
#define BT_GAP_TAG "bt_gap"

esp_bd_addr_t last_remote_bda = {0};

static void bt_gap_event_handler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(BT_GAP_TAG, "authentication success: %s", param->auth_cmpl.device_name);
        } else {
            ESP_LOGE(BT_GAP_TAG, "authentication failed: %d", param->auth_cmpl.stat);
        }
        break;
    default:
        break;
    }
}

void bt_app_init(void)
{
    size_t length = sizeof(esp_bd_addr_t);
    app_getenv("LAST_REMOTE_BDA", &last_remote_bda, &length);

    esp_bt_dev_set_device_name(CONFIG_BT_NAME);
    esp_bt_gap_register_callback(bt_gap_event_handler);

    esp_avrc_ct_init();
    esp_avrc_ct_register_callback(bt_avrc_ct_event_handler);

    esp_avrc_tg_init();
    esp_avrc_tg_register_callback(bt_avrc_tg_event_handler);

    esp_a2d_sink_init();
    esp_a2d_register_callback(bt_a2d_event_handler);
    esp_a2d_sink_register_data_callback(bt_a2d_data_handler);

    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
    esp_bt_pin_code_t pin_code;
    pin_code[0] = '1';
    pin_code[1] = '2';
    pin_code[2] = '3';
    pin_code[3] = '4';
    esp_bt_gap_set_pin(pin_type, 4, pin_code);

    ESP_LOGI(BT_APP_TAG, "started.");

    vTaskDelay(2000 / portTICK_RATE_MS);

    if (memcmp(last_remote_bda, "\x00\x00\x00\x00\x00\x00", 6) != 0) {
        if (!(xEventGroupGetBits(user_event_group) & (OS_PWR_RESET_BIT | OS_PWR_SLEEP_BIT))) {
            ESP_LOGW(BT_APP_TAG, "connecting to [%02x:%02x:%02x:%02x:%02x:%02x]",
                     last_remote_bda[0], last_remote_bda[1], last_remote_bda[2],
                     last_remote_bda[3], last_remote_bda[4], last_remote_bda[5]);

            esp_a2d_sink_connect(last_remote_bda);
        }
    } else {
        xEventGroupSetBits(user_event_group, BT_A2DP_IDLE_BIT);

        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    }
}
