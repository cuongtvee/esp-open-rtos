#ifndef __RUBIK_SERVER_H__
#define  __RUBIK_SERVER_H__
#include <paho_mqtt_c/MQTTESP8266.h>
#include <paho_mqtt_c/MQTTClient.h>
#include <string.h>
#include <config.h>
#define MQTT_HOST ("hmp-assistant.com")
#define MQTT_PORT 1883
#define MQTT_USER ("rdSHxRq5rkaXZqb5Tpna")
#define MQTT_PASS ("rubik@team@2020")
#define attribute_topic ("v1/devices/me/attributes")
#define telemetryTopic  ("v1/devices/me/telemetry")
#define attributesRequestTopic  ("v1/devices/me/attributes/request/1")
#define attributesResponseTopic  ("v1/devices/me/attributes/response/1") // attributesRequestTopic.replace('request', 'response');


int sendattributes(mqtt_client_t* client, char* msg );
int sendTelemetry(mqtt_client_t* c, char* data );
void sensorReadTask(void *pvParameters);

#endif