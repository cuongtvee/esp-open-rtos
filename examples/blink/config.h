#ifndef __CONFIG_H__
#define __CONFIG_H__
#include "espressif/esp_common.h"
#include "espressif/esp_system.h"
#include "esp/uart.h"
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#define DEBUG 1
#if DEBUG
    #define DEBUG_PRINT printf
#else
    #define DEBUG_PRINT
#endif
#define attribute_topic ("v1/devices/me/attributes")
#define telemetryTopic  ("v1/devices/me/telemetry")
#define attributesRequestTopic  ("v1/devices/me/attributes/request/1")
#define attributesResponseTopic  ("v1/devices/me/attributes/response/1") // attributesRequestTopic.replace('request', 'response');


#define START 1
#define STOP 0
#define PUB_MSG_LEN 64

#endif