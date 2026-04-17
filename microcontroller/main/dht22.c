#include "dht22.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define DHT22_TIMEOUT_US 200

static int64_t wait_for_level(int gpio_pin, int level, int64_t timeout_us) {
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(gpio_pin) != level) {
        if ((esp_timer_get_time() - start) > timeout_us) return -1;
    }
    return esp_timer_get_time() - start;
}

bool dht22_read(int gpio_pin, dht22_reading_t *out) {
    uint8_t data[5] = {0};

    // --- send start signal ---
    gpio_set_direction(gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(2));   // pull low for >1ms
    gpio_set_level(gpio_pin, 1);
    esp_rom_delay_us(30);           // pull high for 20-40us
    gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);

    // --- wait for sensor response ---
    if (wait_for_level(gpio_pin, 0, DHT22_TIMEOUT_US) < 0) return false; // sensor pulls low
    if (wait_for_level(gpio_pin, 1, DHT22_TIMEOUT_US) < 0) return false; // sensor pulls high
    if (wait_for_level(gpio_pin, 0, DHT22_TIMEOUT_US) < 0) return false; // sensor pulls low again

    // --- read 40 bits ---
    for (int i = 0; i < 40; i++) {
        if (wait_for_level(gpio_pin, 1, DHT22_TIMEOUT_US) < 0) return false;
        int64_t duration = wait_for_level(gpio_pin, 0, DHT22_TIMEOUT_US);
        if (duration < 0) return false;

        data[i / 8] <<= 1;
        if (duration > 40) data[i / 8] |= 1;  // >40us = bit 1, <40us = bit 0
    }

    // --- verify checksum ---
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) return false;

    // --- decode ---
    out->humidity    = ((data[0] << 8) | data[1]) / 10.0f;
    out->temperature = (((data[2] & 0x7F) << 8) | data[3]) / 10.0f;
    if (data[2] & 0x80) out->temperature *= -1;  // negative temp flag

    return true;
}