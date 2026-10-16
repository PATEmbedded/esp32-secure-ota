idf_component_register(
    SRCS "app_main.c" "wifi.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES secure_ota esp_app_format esp_event esp_netif esp_wifi nvs_flash
    EMBED_TXTFILES "certs/server_ca.pem"
)
