#ifndef DHT22_H
#define DHT22_H

#include <stdbool.h>

typedef struct {
    float temperature;
    float humidity;
} dht22_reading_t;

typedef enum {
    DHT22_OK            = 0,
    DHT22_ERR_TIMEOUT   = 1,  // sensor didn't respond in time
    DHT22_ERR_CHECKSUM  = 2,  // data corrupted (timing interference)
    DHT22_ERR_RANGE     = 3,  // values outside physical limits
} dht22_err_t;

const char *dht22_err_str(dht22_err_t err);
dht22_err_t dht22_read(int gpio_pin, dht22_reading_t *out);

#endif