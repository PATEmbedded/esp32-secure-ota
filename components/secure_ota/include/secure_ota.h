// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Pourya Afshintabar
//
// Downloads, verifies and installs a signed application image over HTTPS, and
// confirms or rolls back a freshly installed image after its self-test.

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Where to look for updates and how to trust the server. */
typedef struct {
    /** HTTPS URL of the signed application image. Plain HTTP is refused. */
    const char *firmware_url;
    /**
     * CA certificate (PEM) that must have issued the server certificate, or
     * NULL to accept the public CAs in the ESP-IDF certificate bundle.
     */
    const char *server_ca_pem;
    /** Network timeout in milliseconds. */
    int timeout_ms;
} secure_ota_config_t;

/** Outcome of secure_ota_update(). */
typedef enum {
    SECURE_OTA_INSTALLED,  /**< A newer image was installed; it runs after a restart. */
    SECURE_OTA_UP_TO_DATE, /**< The server has the running version or an older one. */
    SECURE_OTA_REJECTED,   /**< The image is for another project, has no valid version or a revoked security version. */
    SECURE_OTA_FAILED,     /**< Network, TLS, flash or image verification error. */
} secure_ota_result_t;

/**
 * Checks the server for a newer image and installs it.
 *
 * The image header is read first, so an image for another project or with an
 * older version is rejected before the rest is downloaded. The image is
 * written to the inactive OTA slot. Its hash and, with signed apps enabled,
 * its signature are checked before that slot is selected for the next boot.
 */
secure_ota_result_t secure_ota_update(const secure_ota_config_t *config);

/**
 * Confirms a freshly installed image, or rolls back to the previous one.
 *
 * Call it once at start-up with the result of the app's self-test. It does
 * nothing unless the running image is still waiting for verification. With
 * `healthy == false` it marks the image invalid and restarts into the
 * previous one.
 */
void secure_ota_confirm_boot(bool healthy);

#ifdef __cplusplus
}
#endif
