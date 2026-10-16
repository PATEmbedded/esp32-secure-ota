// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Pourya Afshintabar

#include "secure_ota.h"

#include <inttypes.h>
#include <stdint.h>

#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "sdkconfig.h"
#include "secure_ota_version.h"

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include "esp_efuse.h"
#endif

static const char *TAG = "secure_ota";

/* Decides from the image header alone whether the download may continue. */
static bool accept_image(const esp_app_desc_t *running, const esp_app_desc_t *candidate,
                         secure_ota_result_t *reason) {
    switch (sota_decide(running->project_name, running->version, candidate->project_name, candidate->version)) {
        case SOTA_INSTALL:
            break;
        case SOTA_NOT_NEWER:
            ESP_LOGI(TAG, "Server has %s, running %s: up to date", candidate->version, running->version);
            *reason = SECURE_OTA_UP_TO_DATE;
            return false;
        case SOTA_OTHER_PROJECT:
            ESP_LOGW(TAG, "Image is for project \"%.32s\", not \"%.32s\"", candidate->project_name,
                     running->project_name);
            *reason = SECURE_OTA_REJECTED;
            return false;
        case SOTA_BAD_VERSION:
        default:
            ESP_LOGW(TAG, "Versions \"%.32s\" and \"%.32s\" are not both semantic versions", candidate->version,
                     running->version);
            *reason = SECURE_OTA_REJECTED;
            return false;
    }

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
    if (!esp_efuse_check_secure_version(candidate->secure_version)) {
        ESP_LOGW(TAG, "Image security version %" PRIu32 " is lower than the one stored in eFuse",
                 candidate->secure_version);
        *reason = SECURE_OTA_REJECTED;
        return false;
    }
#endif
    return true;
}

static void log_progress(esp_https_ota_handle_t handle, int image_size, int *last_step) {
    if (image_size <= 0) {
        return;
    }
    const int percent = (int)((int64_t)esp_https_ota_get_image_len_read(handle) * 100 / image_size);
    if (percent / 10 != *last_step) {
        *last_step = percent / 10;
        ESP_LOGI(TAG, "Downloaded %d%%", percent);
    }
}

secure_ota_result_t secure_ota_update(const secure_ota_config_t *config) {
    const esp_http_client_config_t http_config = {
        .url = config->firmware_url,
        .cert_pem = config->server_ca_pem,
        .crt_bundle_attach = config->server_ca_pem == NULL ? esp_crt_bundle_attach : NULL,
        .timeout_ms = config->timeout_ms,
        .keep_alive_enable = true,
    };
    const esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    esp_https_ota_handle_t handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Cannot open %s: %s", config->firmware_url, esp_err_to_name(err));
        return SECURE_OTA_FAILED;
    }

    esp_app_desc_t candidate;
    err = esp_https_ota_get_img_desc(handle, &candidate);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Cannot read the image header: %s", esp_err_to_name(err));
        esp_https_ota_abort(handle);
        return SECURE_OTA_FAILED;
    }

    secure_ota_result_t reason = SECURE_OTA_REJECTED;
    if (!accept_image(esp_app_get_description(), &candidate, &reason)) {
        esp_https_ota_abort(handle);
        return reason;
    }

    ESP_LOGI(TAG, "Downloading %.32s %.32s", candidate.project_name, candidate.version);
    const int image_size = esp_https_ota_get_image_size(handle);
    int last_step = -1;
    while ((err = esp_https_ota_perform(handle)) == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
        log_progress(handle, image_size, &last_step);
    }
    if (err != ESP_OK || !esp_https_ota_is_complete_data_received(handle)) {
        ESP_LOGE(TAG, "Download failed: %s", esp_err_to_name(err));
        esp_https_ota_abort(handle);
        return SECURE_OTA_FAILED;
    }

    /*
     * Verifies the image hash and, with signed apps enabled, the signature,
     * then selects the new slot for the next boot. It also frees the handle.
     */
    err = esp_https_ota_finish(handle);
    if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
        ESP_LOGE(TAG, "Image rejected: it is corrupt or not signed with the release key");
        return SECURE_OTA_FAILED;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Cannot complete the update: %s", esp_err_to_name(err));
        return SECURE_OTA_FAILED;
    }

    ESP_LOGI(TAG, "Installed %.32s; it runs after a restart", candidate.version);
    return SECURE_OTA_INSTALLED;
}

void secure_ota_confirm_boot(bool healthy) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) != ESP_OK || state != ESP_OTA_IMG_PENDING_VERIFY) {
        return; /* not a fresh update: nothing to confirm */
    }
    if (healthy) {
        ESP_LOGI(TAG, "New image passed its self-test; keeping it");
        esp_ota_mark_app_valid_cancel_rollback();
    } else {
        ESP_LOGE(TAG, "New image failed its self-test; returning to the previous image");
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }
}
