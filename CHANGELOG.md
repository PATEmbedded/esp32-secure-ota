# Changelog

## 1.0.0 (2026-10-16)

First release. It replaces the earlier `OTA` and `OTA_BT_Update` sketches.

- `secure_ota` component: HTTPS download into the inactive slot, project and semantic-version checks on the image header, optional eFuse anti-rollback, hash and signature verification before the slot switch
- Boot confirmation with automatic rollback when the self-test fails
- Demo app with Wi-Fi, a network self-test and periodic update checks
- Signed images without burning eFuses: ECDSA P-256 on ESP32, RSA-3072 on ESP32-S3
- Threat model, test certificate script and a local HTTPS server for development
- Host unit tests and CI builds for ESP-IDF 5.5 and 6.0
