#ifndef MQTT_H
#define MQTT_H

void mqtt_app_start(const char *broker_uri);
void mqtt_wait_connected(void);
void mqtt_publish(const char *topic, const char *payload);

#endif