#include "mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "sdkconfig.h"

static const char *TAG = "mqtt";
static esp_mqtt_client_handle_t s_client = NULL;

#define MQTT_CONNECTED_BIT BIT0
static EventGroupHandle_t s_mqtt_event_group;

extern const uint8_t isrgrootx1_pem_start[] asm("_binary_isrgrootx1_pem_start");
extern const uint8_t isrgrootx1_pem_end[]   asm("_binary_isrgrootx1_pem_end");

static void mqtt_event_handler(void *arg, esp_event_base_t base,
                                int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to broker");
            xEventGroupSetBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Disconnected from broker");
            xEventGroupClearBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;
        default:
            break;
    }
}

void mqtt_app_start(const char *broker_uri) {
    s_mqtt_event_group = xEventGroupCreate();

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = broker_uri,
        .broker.verification.certificate = (const char *)isrgrootx1_pem_start,
        .broker.verification.certificate_len = isrgrootx1_pem_end - isrgrootx1_pem_start,
        .credentials.username = CONFIG_MQTT_USERNAME,
        .credentials.authentication.password = CONFIG_MQTT_PASSWORD
    };
    s_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_client);
}

void mqtt_wait_connected(void) {
    xEventGroupWaitBits(s_mqtt_event_group, MQTT_CONNECTED_BIT,
                        false, true, portMAX_DELAY);
}

void mqtt_publish(const char *topic, const char *payload) {
    if (s_client == NULL) return;
    esp_mqtt_client_publish(s_client, topic, payload, 0, 1, 0);
}