#ifndef DHT22_H
#define DHT22_H

#include <stdbool.h>

typedef struct {
    float temperature;
    float humidity;
} dht22_reading_t;

bool dht22_read(int gpio_pin, dht22_reading_t *out);

#endif