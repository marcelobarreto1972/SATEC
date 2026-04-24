#include "dht22.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define DHT22_TIMEOUT_US 2000

static int64_t wait_for_level(int gpio_pin, int level, int64_t timeout_us) {
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(gpio_pin) != level) {
        if ((esp_timer_get_time() - start) > timeout_us) return -1;
    }
    return esp_timer_get_time() - start;
}

dht22_err_t dht22_read(int gpio_pin, dht22_reading_t *out) {
    uint8_t data[5] = {0};

    // wake-up: ensure line settles high before pulling low
    gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);
    esp_rom_delay_us(500);

    // send start signal
    gpio_set_direction(gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(20));  // hold low 20ms
    gpio_set_level(gpio_pin, 1);
    esp_rom_delay_us(40);           // hold high 40us
    gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);

    portDISABLE_INTERRUPTS();

    bool timeout = false;
    if (wait_for_level(gpio_pin, 0, DHT22_TIMEOUT_US) < 0) timeout = true;
    if (!timeout && wait_for_level(gpio_pin, 1, DHT22_TIMEOUT_US) < 0) timeout = true;
    if (!timeout && wait_for_level(gpio_pin, 0, DHT22_TIMEOUT_US) < 0) timeout = true;

    if (!timeout) {
        for (int i = 0; i < 40; i++) {
            if (wait_for_level(gpio_pin, 1, DHT22_TIMEOUT_US) < 0) { timeout = true; break; }
            int64_t duration = wait_for_level(gpio_pin, 0, DHT22_TIMEOUT_US);
            if (duration < 0) { timeout = true; break; }
            data[i / 8] <<= 1;
            if (duration > 40) data[i / 8] |= 1;
        }
    }

    portENABLE_INTERRUPTS();

    if (timeout) return DHT22_ERR_TIMEOUT;

    // checksum
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) return DHT22_ERR_CHECKSUM;

    float humidity    = ((data[0] << 8) | data[1]) / 10.0f;
    float temperature = (((data[2] & 0x7F) << 8) | data[3]) / 10.0f;
    if (data[2] & 0x80) temperature *= -1;

    // sanity check — DHT22 physical limits
    if (humidity < 0.0f || humidity > 100.0f) return DHT22_ERR_RANGE;
    if (temperature < -40.0f || temperature > 80.0f) return DHT22_ERR_RANGE;

    out->humidity    = humidity;
    out->temperature = temperature;
    return DHT22_OK;
}

const char *dht22_err_str(dht22_err_t err) {
    switch (err) {
        case DHT22_OK:           return "OK";
        case DHT22_ERR_TIMEOUT:  return "TIMEOUT";
        case DHT22_ERR_CHECKSUM: return "CHECKSUM";
        case DHT22_ERR_RANGE:    return "RANGE";
        default:                 return "UNKNOWN";
    }
}