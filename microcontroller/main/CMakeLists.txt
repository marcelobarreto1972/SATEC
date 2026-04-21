idf_component_register(
    SRCS "main.c" "dht22.c" "wifi.c" "mqtt.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi nvs_flash esp_event esp_netif mqtt
    PRIV_REQUIRES esp_driver_gpio
    EMBED_TXTFILES certs/isrgrootx1.pem
)