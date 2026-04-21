#include "mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"

static const char *TAG = "mqtt";
static esp_mqtt_client_handle_t s_client = NULL;

extern const uint8_t isrgrootx1_pem_start[] asm("_binary_isrgrootx1_pem_start");
extern const uint8_t isrgrootx1_pem_end[]   asm("_binary_isrgrootx1_pem_end");

static void mqtt_event_handler(void *arg, esp_event_base_t base,
                                int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to broker");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Disconnected from broker");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;
        default:
            break;
    }
}

void mqtt_app_start(const char *broker_uri) {
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = broker_uri,
        .broker.verification.certificate = (const char *)isrgrootx1_pem_start,
        .broker.verification.certificate_len = isrgrootx1_pem_end - isrgrootx1_pem_start,
        .credentials.username = "satec",
        .credentials.authentication.password = ""
    };
    s_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_client);
}

void mqtt_publish(const char *topic, const char *payload) {
    if (s_client == NULL) return;
    esp_mqtt_client_publish(s_client, topic, payload, 0, 1, 0);
}