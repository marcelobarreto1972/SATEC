#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht22.h"

#define DHT22_GPIO 4

void app_main(void) {
    dht22_reading_t reading;
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    while (1) {
        if (dht22_read(DHT22_GPIO, &reading)) {
            printf("Temp: %.1f C  Humidity: %.1f%%\n",
                   reading.temperature, reading.humidity);
        } else {
            printf("Failed to read from DHT22\n");
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}