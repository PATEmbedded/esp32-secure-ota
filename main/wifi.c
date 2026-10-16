// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Pourya Afshintabar

#include "wifi.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "sdkconfig.h"

#define CONNECTED_BIT BIT0

static const char *TAG = "wifi";
static EventGroupHandle_t s_events;

static void on_event(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    if (base == WIFI_EVENT && (id == WIFI_EVENT_STA_START || id == WIFI_EVENT_STA_DISCONNECTED)) {
        xEventGroupClearBits(s_events, CONNECTED_BIT);
        esp_wifi_connect(); /* keep trying: the access point may come back */
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = data;
        ESP_LOGI(TAG, "Connected, address " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_events, CONNECTED_BIT);
    }
}

esp_err_t wifi_connect(uint32_t timeout_ms) {
    s_events = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    const wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_event, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_event, NULL, NULL));

    wifi_config_t config = {
        .sta =
            {
                .ssid = CONFIG_SOTA_WIFI_SSID,
                .password = CONFIG_SOTA_WIFI_PASSWORD,
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
    ESP_ERROR_CHECK(esp_wifi_start());

    const EventBits_t bits = xEventGroupWaitBits(s_events, CONNECTED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return (bits & CONNECTED_BIT) != 0 ? ESP_OK : ESP_ERR_TIMEOUT;
}

bool wifi_is_connected(void) {
    return s_events != NULL && (xEventGroupGetBits(s_events) & CONNECTED_BIT) != 0;
}
