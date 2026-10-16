// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Pourya Afshintabar
//
// Wi-Fi station that reconnects on its own after the access point drops.

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * Starts Wi-Fi with the credentials from menuconfig and waits for an IP
 * address. Call it once.
 *
 * @return ESP_OK when connected, ESP_ERR_TIMEOUT when no address arrived in
 *         time. The station keeps trying in the background either way.
 */
esp_err_t wifi_connect(uint32_t timeout_ms);

/** True while the station has an IP address. */
bool wifi_is_connected(void);
