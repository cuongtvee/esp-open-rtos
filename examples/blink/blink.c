#include <stdio.h>
#include <stdlib.h>
#include <esp/uart.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include "espressif/esp_common.h"
#include "espressif/esp_system.h"
#include <paho_mqtt_c/MQTTESP8266.h>
#include <paho_mqtt_c/MQTTClient.h>
#include <ssid_config.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_wifi.h>

#include <semphr.h>
#include <button.h>
#include "config.h"
#include "rubik_server.h"
#include "utils.h"

#define BUTTON1 0
#define BUTTON2 5

#define RELAY_1 2
#define RELAY_2 12

QueueHandle_t publish_queue;
SemaphoreHandle_t wifi_alive;

uint8_t button_idx1 = 1;
uint8_t button_idx2 = 2;

void delay_ms(int ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}
mqtt_client_t client = mqtt_client_default;
const char mqtt_retry_time = 5; // unit in second

static void topic_received(mqtt_message_data_t *md)
{
    int i;
    mqtt_message_t *message = md->message;
#if DEBUG    
    printf("Received: ");
    for (i = 0; i < md->topic->lenstring.len; ++i)
        printf("%c", md->topic->lenstring.data[i]);

    printf(" = ");
    for (i = 0; i < (int)message->payloadlen; ++i)
        printf("%c", ((char *)(message->payload))[i]);

    printf("\r\n");
#endif

}
static void topic_getstatus(mqtt_message_data_t *md)
{
    int i;
    mqtt_message_t *message = md->message;
    DEBUG_PRINT("topic_getstatus: Received: ");

    for (i = 0; i < md->topic->lenstring.len; ++i)
        DEBUG_PRINT("%c", md->topic->lenstring.data[i]);

    for (i = 0; i < (int)message->payloadlen; ++i)
        DEBUG_PRINT("%c", ((char *)(message->payload))[i]);

    DEBUG_PRINT("\r\n");

    // processing message
    mqtt_message_t r_message;
    char msg1[PUB_MSG_LEN - 1];

    if (strstr((char *)(message->payload), "set_sw1") != NULL)
    {
        if (strstr((char *)(message->payload), "true") != NULL)
        {
            // set gpio = 1
            gpio_write(RELAY_1, 1);            
            printf("Start relay 1 \n ");
        }
        else if (strstr((char *)(message->payload), "false") != NULL)
        {
            gpio_write(RELAY_1, 0);
            printf("Stop relay 1 \n");
        }
        snprintf(msg1, sizeof(msg1), "{\"sw1\": %s }", gpio_read(RELAY_1) ? "true" : "false");
    }
    else if (strstr((char *)(message->payload), "set_sw2") != NULL)
    {
        if (strstr((char *)(message->payload), "true") != NULL)
        {
            // set gpio = 1
            gpio_write(RELAY_2, 1);            
            printf("Start relay 2 \n ");
        }
        else if (strstr((char *)(message->payload), "false") != NULL)
        {
            gpio_write(RELAY_2, 0);
            printf("Stop relay 2 \n");
        }
        snprintf(msg1, sizeof(msg1), "{\"sw2\": %s }", gpio_read(RELAY_2) ? "true" : "false");
    }
    else
    {
        return;
    }
    
    printf("Publishing: %s\r\n", msg1);
    r_message.payload = (void *)msg1;
    r_message.payloadlen = strlen(msg1);
    r_message.dup = 0;
    r_message.qos = MQTT_QOS1;
    r_message.retained = 0;

    const char *request_topic[md->topic->lenstring.len + 1];
    snprintf(request_topic, md->topic->lenstring.len + 1, "%s", (char *)(md->topic->lenstring.data));
    const char *respone_topic = replaceWord(request_topic, "request", "response");

    mqtt_publish(&client, respone_topic, &r_message);
    mqtt_publish(&client, telemetryTopic, &r_message);

    // else if (strstr((char *)(message->payload),"getValue") != NULL)
    // {
    //     const char* request_topic[md->topic->lenstring.len+1];
    //     snprintf(request_topic,md->topic->lenstring.len+1,"%s",(char *)(md->topic->lenstring.data));
    //     const char* respone_topic =  replaceWord(request_topic,"request","response");
    //     mqtt_message_t r_message;
    //     char msg1[PUB_MSG_LEN - 1];
    //     snprintf(msg1, sizeof(msg1), "{\"status\": %s }", gpio_read(RELAY_1)?"true":"false");
    //     r_message.payload = (void*)msg1;
    //     r_message.payloadlen = strlen(msg1);
    //     r_message.dup = 0;
    //     r_message.qos = MQTT_QOS1;
    //     r_message.retained = 0;
    //     mqtt_publish(&client,respone_topic,&r_message);

    // }
}
static void mqtt_task(void *pvParameters)
{
    int ret = 0;
    struct mqtt_network network;

    char mqtt_client_id[20];
    uint8_t mqtt_buf[100];
    uint8_t mqtt_readbuf[100];
    mqtt_packet_connect_data_t data = mqtt_packet_connect_data_initializer;

    mqtt_network_new(&network);
    memset(mqtt_client_id, 0, sizeof(mqtt_client_id));
    strcpy(mqtt_client_id, "ESP-");
    strcat(mqtt_client_id, get_my_id());

    while (1)
    {
        xSemaphoreTake(wifi_alive, portMAX_DELAY);
        DEBUG_PRINT("%s: started\n\r", __func__);
        DEBUG_PRINT("%s: (Re)connecting to MQTT server %s ... ", __func__,
                    MQTT_HOST);
        ret = mqtt_network_connect(&network, MQTT_HOST, MQTT_PORT);
        if (ret)
        {
            DEBUG_PRINT("error: %d\n\r", ret);
            taskYIELD();
            continue;
        }
        DEBUG_PRINT("done\n\r");

        mqtt_client_new(&client, &network, 5000, mqtt_buf, 100,
                        mqtt_readbuf, 100);

        data.willFlag = 0;
        data.MQTTVersion = 3;
        data.clientID.cstring = mqtt_client_id;
        data.username.cstring = MQTT_USER;
        data.password.cstring = MQTT_PASS;
        data.keepAliveInterval = 10;
        data.cleansession = 0;
        DEBUG_PRINT("Send MQTT connect ... ");
        ret = mqtt_connect(&client, &data);
        if (ret)
        {
            DEBUG_PRINT("error: %d\n\r", ret);
            mqtt_network_disconnect(&network);
            taskYIELD();
            vTaskDelay(mqtt_retry_time * 1000 / portTICK_PERIOD_MS);
            DEBUG_PRINT("reconnect mqtt after %d seconds\n", mqtt_retry_time);
            continue;
        }
        DEBUG_PRINT("done\r\n");
        // mqtt_subscribe(&client, attribute_topic, MQTT_QOS1, topic_received);
        mqtt_subscribe(&client, "v1/devices/me/rpc/request/+", MQTT_QOS1, topic_getstatus);
        delay_ms(1000);
        // Public button status
        char msg1[PUB_MSG_LEN - 1];
        snprintf(msg1, sizeof(msg1), "{\"sw1\": %s }", gpio_read(RELAY_1) ? "true" : "false");
        DEBUG_PRINT("Publishing: %s\r\n", msg1);
        sendTelemetry(&client, msg1);

        snprintf(msg1, sizeof(msg1), "{\"sw2\": %s }", gpio_read(RELAY_2) ? "true" : "false");
        DEBUG_PRINT("Publishing: %s\r\n", msg1);
        sendTelemetry(&client, msg1);

        if (ret != MQTT_SUCCESS)
        {
            DEBUG_PRINT("error while publishing -message: %d\n", ret);
        }
        xQueueReset(publish_queue);
        while (1)
        {
            char msg[PUB_MSG_LEN - 1];
            while (xQueueReceive(publish_queue, (void *)msg, 0) ==
                   pdTRUE)
            {

                DEBUG_PRINT("Publishing: %s\r\n", msg);
                sendattributes(&client, msg);
                if (ret != MQTT_SUCCESS)
                {
                    DEBUG_PRINT("error while publishing message: %d\n", ret);
                    break;
                }
            }
            ret = mqtt_yield(&client, 1000);
            if (ret == MQTT_DISCONNECTED)
                break;
        }
        DEBUG_PRINT("Connection dropped, request restart\n\r");
        mqtt_network_disconnect(&network);
        taskYIELD();
    }
}

void button_callback(button_event_t event, void *context)
{
    int button_idx = *((uint8_t *)context);

    switch (event)
    {
    case button_event_single_press:
        printf("button %d single press\n", button_idx);
        char msg1[PUB_MSG_LEN - 1];
        mqtt_message_t r_message;
        if (button_idx == button_idx1)
        {
            gpio_toggle(RELAY_1);
            delay_ms(200);
            snprintf(msg1, sizeof(msg1), "{\"sw1\": %s }", gpio_read(RELAY_1) ? "1" : "0");
        }
        else if (button_idx == button_idx2)
        {
            gpio_toggle(RELAY_2);
            delay_ms(200);
            snprintf(msg1, sizeof(msg1), "{\"sw2\": %s }", gpio_read(RELAY_2) ? "1" : "0");
        }
        else
        {
            break;
        }
        printf("Publishing: %s\r\n", msg1);
        r_message.payload = (void *)msg1;
        r_message.payloadlen = strlen(msg1);
        r_message.dup = 0;
        r_message.qos = MQTT_QOS1;
        r_message.retained = 0;
        bool ret = mqtt_publish(&client, telemetryTopic, &r_message);

        break;
    case button_event_double_press:
        DEBUG_PRINT("button %d double press\n", button_idx);
        break;
    case button_event_tripple_press:
        DEBUG_PRINT("button %d tripple press\n", button_idx);
        break;
    case button_event_long_press:
        DEBUG_PRINT("button %d long press\n", button_idx);
        break;
    default:
        DEBUG_PRINT("unexpected button %d event: %d\n", button_idx, event);
    }
}

static void wifi_task(void *pvParameters)
{
    uint8_t status = 0;
    uint8_t retries = 30;
    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };

    printf("WiFi: connecting to WiFi\n\r");
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    while (1)
    {
        while ((status != STATION_GOT_IP) && (retries))
        {
            status = sdk_wifi_station_get_connect_status();
            printf("%s: status = %d\n\r", __func__, status);
            if (status == STATION_WRONG_PASSWORD)
            {
                printf("WiFi: wrong password\n\r");
                break;
            }
            else if (status == STATION_NO_AP_FOUND)
            {
                printf("WiFi: AP not found\n\r");
                break;
            }
            else if (status == STATION_CONNECT_FAIL)
            {
                printf("WiFi: connection failed\r\n");
                break;
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            --retries;
        }
        if (status == STATION_GOT_IP)
        {
            printf("WiFi: Connected\n\r");
            xSemaphoreGive(wifi_alive);
            taskYIELD();
        }

        while ((status = sdk_wifi_station_get_connect_status()) == STATION_GOT_IP)
        {
            xSemaphoreGive(wifi_alive);
            taskYIELD();
        }
        printf("WiFi: disconnected\n\r");
        sdk_wifi_station_disconnect();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
static void beat_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    char msg[PUB_MSG_LEN];
    int count = 0;

    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, 10000 / portTICK_PERIOD_MS);
        // printf("beat\r\n");
        // snprintf(msg, PUB_MSG_LEN, " { \" data \"%d\r\n", count++);
        count++;
        snprintf(msg, sizeof(msg), "{\"data\": %d }", count++);
        if (xQueueSend(publish_queue, (void *)msg, 0) == pdFALSE)
        {
            printf("Publish queue overflow.\r\n");
        }
    }
}
void user_init(void)
{
    uart_set_baud(0, 115200);

    gpio_enable(RELAY_1, GPIO_OUTPUT);
    gpio_enable(RELAY_2, GPIO_OUTPUT);
    button_config_t button_config = BUTTON_CONFIG(
        button_active_low,
        .long_press_time = 1000,
        .max_repeat_presses = 3, );

    int r;
    r = button_create(BUTTON1, button_config, button_callback, &button_idx1);
    if (r)
    {
        printf("Failed to initialize button %d (code %d)\n", button_idx1, r);
    }

    r = button_create(BUTTON2, button_config, button_callback, &button_idx2);
    if (r)
    {
        printf("Failed to initialize button %d (code %d)\n", button_idx1, r);
    }
    vSemaphoreCreateBinary(wifi_alive);
    publish_queue = xQueueCreate(3, PUB_MSG_LEN);
    xTaskCreate(&wifi_task, "wifi_task", 256, NULL, 2, NULL);
    xTaskCreate(&beat_task, "beat_task", 256, NULL, 3, NULL);
    xTaskCreate(&mqtt_task, "mqtt_task", 2048, NULL, 2, NULL);

    // xTaskCreate(idle_task, "Idle task", 256, NULL, tskIDLE_PRIORITY, NULL);
}