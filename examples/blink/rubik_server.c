#include "rubik_server.h"
// #define attribute_topic = "v1/devices/me/attributes"
// const char* telemetryTopic = "v1/devices/me/telemetry";
// const char* attributesRequestTopic = "v1/devices/me/attributes/request/1";
// const char* attributesResponseTopic = "v1/devices/me/attributes/response/1";// attributesRequestTopic.replace('request', 'response');
#define SENSOR_FULL_LEVEL   9
#define SENSOR_EMPTY_LEVEL   10

int sendTelemetry(mqtt_client_t* client, char* msg ){
    mqtt_message_t message;
    message.payload = (void*)msg;
    message.payloadlen = strlen(msg);
    message.dup = 0;
    message.qos = MQTT_QOS1;
    message.retained = 0;
    return mqtt_publish(client, telemetryTopic, &message);
}
int sendattributes(mqtt_client_t* client, char* msg ){
    mqtt_message_t message;
    message.payload = (void*)msg;
    message.payloadlen = strlen(msg);
    message.dup = 0;
    message.qos = MQTT_QOS1;
    message.retained = 0;
    return mqtt_publish(client, attribute_topic, &message);
}
//
void sensorReadTask(void *pvParameters){
    while (1)
    {   
        int8_t feeding_status; 
        bool sensor_empty = gpio_read(SENSOR_EMPTY_LEVEL);
        bool sensor_full = gpio_read(SENSOR_FULL_LEVEL);
        if (sensor_empty==0 && sensor_full==0) feeding_status=-1;     // empty
        else if (sensor_empty==1 && sensor_full==0) feeding_status=0; // normal
        else if (sensor_empty==1 && sensor_full==1) feeding_status=1; // FULL
        else feeding_status = -2; // error, 0,1        
        printf("Start sensorReadTask \n");
        printf("Feeding status: %d\n",feeding_status);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}