// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Pourya Afshintabar
//
// Demo application: joins Wi-Fi, confirms or rolls back a freshly installed
// image, and checks the update server periodically.

#include <stdbool.h>
#include <stddef.h>

#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "secure_ota.h"
#include "wifi.h"

static const char *TAG = "app";

#if CONFIG_SOTA_TRUST_PINNED_CA
extern const char server_ca_pem_start[] asm("_binary_server_ca_pem_start");
#endif

static void init_nvs(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static void update_task(void *arg) {
    (void)arg;
    const secure_ota_config_t config = {
        .firmware_url = CONFIG_SOTA_FIRMWARE_URL,
#if CONFIG_SOTA_TRUST_PINNED_CA
        .server_ca_pem = server_ca_pem_start,
#else
        .server_ca_pem = NULL,
#endif
        .timeout_ms = CONFIG_SOTA_HTTP_TIMEOUT_MS,
    };

    for (;;) {
        if (wifi_is_connected() && secure_ota_update(&config) == SECURE_OTA_INSTALLED) {
            ESP_LOGI(TAG, "Restarting into the new image");
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_restart();
        }
        for (int second = 0; second < CONFIG_SOTA_CHECK_INTERVAL_S; ++second) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void app_main(void) {
    const esp_app_desc_t *app = esp_app_get_description();
    ESP_LOGI(TAG, "%s %s", app->project_name, app->version);

    init_nvs();

    // Self-test: a freshly installed image has to reach the network, so that it
    // can always be replaced again. Otherwise the previous image comes back.
    const bool online = wifi_connect(CONFIG_SOTA_WIFI_TIMEOUT_MS) == ESP_OK;
    secure_ota_confirm_boot(online);
    if (!online) {
        ESP_LOGW(TAG, "No network yet; still trying in the background");
    }

    xTaskCreate(update_task, "ota_update", 8192, NULL, 5, NULL);
}
