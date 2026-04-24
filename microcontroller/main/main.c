#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sntp.h"
#include "dht22.h"
#include "wifi.h"
#include "mqtt.h"
#include "driver/gpio.h"

#define DHT22_GPIO      4
#define WIFI_SSID       CONFIG_WIFI_SSID
#define WIFI_PASSWORD   CONFIG_WIFI_PASSWORD
#define MQTT_BROKER_URI CONFIG_MQTT_BROKER_URI
#define MQTT_TOPIC      "sensors/dht22"

static void sync_time(void) {
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    time_t now = 0;
    struct tm timeinfo = {0};
    while (timeinfo.tm_year < (2020 - 1900)) {
        vTaskDelay(pdMS_TO_TICKS(500));
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

void dht22_task(void *pvParameters) {
    dht22_reading_t reading;
    char payload[96];

    // Sanity check: confirm GPIO4 reaches DATA line before reading
    gpio_set_direction(DHT22_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT22_GPIO, 0);
    esp_rom_delay_us(100);
    gpio_set_direction(DHT22_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us(100);
    int idle = gpio_get_level(DHT22_GPIO);
    printf("DHT22 DATA idle check: %d (expected 1)\n", idle);

    while (1) {
        dht22_err_t err = DHT22_OK;
        bool ok = false;

        for (int attempt = 0; attempt < 3 && !ok; attempt++) {
            err = dht22_read(DHT22_GPIO, &reading);
            if (err == DHT22_OK) {
                ok = true;
            } else {
                printf("DHT22 attempt %d failed: %s\n",
                       attempt + 1, dht22_err_str(err));
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
        }

        if (!ok) {
            printf("DHT22 gave up after 3 attempts, last error: %s\n",
                   dht22_err_str(err));
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);

        char timestamp[32];
        strftime(timestamp, sizeof(timestamp),
                 "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

        snprintf(payload, sizeof(payload),
                 "{\"temp\":%.1f,\"humidity\":%.1f,\"timestamp\":\"%s\"}",
                 reading.temperature, reading.humidity, timestamp);

        mqtt_publish(MQTT_TOPIC, payload);
        printf("Published: %s\n", payload);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void) {
    wifi_connect(WIFI_SSID, WIFI_PASSWORD);
    sync_time();
    mqtt_app_start(MQTT_BROKER_URI);

    vTaskDelay(pdMS_TO_TICKS(3000));

    xTaskCreatePinnedToCore(dht22_task, "dht22", 4096, NULL, 5, NULL, 1);
}