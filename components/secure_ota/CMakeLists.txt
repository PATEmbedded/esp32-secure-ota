idf_component_register(
    SRCS "secure_ota.c" "secure_ota_version.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES app_update efuse esp_app_format esp_http_client esp_https_ota log mbedtls
)
