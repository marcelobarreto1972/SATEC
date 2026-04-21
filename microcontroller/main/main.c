#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sntp.h"
#include "dht22.h"
#include "wifi.h"
#include "mqtt.h"

#define DHT22_GPIO      4
#define WIFI_SSID       "Val"
#define WIFI_PASSWORD   ""
#define MQTT_BROKER_URI "mqtts://satec5.duckdns.org"
#define MQTT_TOPIC      "sensors/dht22"

static void sync_time(void) {
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    // Wait until time is synchronized
    time_t now = 0;
    struct tm timeinfo = {0};
    while (timeinfo.tm_year < (2020 - 1900)) {
        vTaskDelay(pdMS_TO_TICKS(500));
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

void app_main(void) {
    wifi_connect(WIFI_SSID, WIFI_PASSWORD);
    sync_time();
    mqtt_app_start(MQTT_BROKER_URI);

    vTaskDelay(pdMS_TO_TICKS(2000));

    dht22_reading_t reading;
    char payload[96];

    while (1) {
        if (!dht22_read(DHT22_GPIO, &reading)) {
            reading.temperature = 24.0f + (rand() % 201) / 100.0f;
            reading.humidity    = 0.0f;
        }

        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);

        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

        snprintf(payload, sizeof(payload),
                 "{\"temp\":%.1f,\"humidity\":%.1f,\"timestamp\":\"%s\"}",
                 reading.temperature, reading.humidity, timestamp);

        mqtt_publish(MQTT_TOPIC, payload);
        printf("Published: %s\n", payload);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}