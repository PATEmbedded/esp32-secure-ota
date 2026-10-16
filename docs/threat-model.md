# Threat model

What the update path protects against, and what is left to the hardware security features of the chip.

## Assets

- The application image the device runs
- The ability to update the device again later

## Threats and mitigations

| Threat | Mitigation |
|---|---|
| Someone on the network serves their own image (man in the middle, DNS spoofing) | TLS with server certificate checks against the ESP-IDF certificate bundle or a single pinned CA. Plain HTTP is refused. |
| A compromised or misconfigured server serves a modified image | The image must carry a valid signature from the release key. `esp_ota_end()` checks it before the boot partition changes. |
| A corrupted download, or power loss during the write | The image goes to the inactive slot while the running one stays untouched. Its hash is checked before the slot is selected, so a partial image never boots. |
| A downgrade to an older version with known bugs | The image header is read first, and images that are not newer than the running app are rejected. With `CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK`, an eFuse security version also blocks old images that carry a valid signature. |
| An image built for another product | The project name in the image header must match the running app. |
| A new image that boots but cannot reach the network, and so could never be fixed remotely | Rollback: a new image starts in the pending-verify state and must join the network within a time limit, or the bootloader returns to the previous image. |

## Not covered by the default configuration

- **Physical access.** Without Secure Boot V2 the bootloader and app can be replaced over the serial port, and without flash encryption the image and the Wi-Fi credentials in NVS can be read from flash. Both are one-way eFuse settings for production devices.
- **The release key.** Anyone with the signing key can produce images the device accepts. Keep the key out of the repository (`.gitignore` excludes it), for example in a hardware security module or the secret store of the build system.
- **Availability.** An attacker who blocks the connection can delay updates, but cannot install anything.
