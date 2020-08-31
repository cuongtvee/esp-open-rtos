#ifndef __UTILS_H__
#define __UTILS_H__
#include <string.h>
#include "espressif/esp_common.h"
#include "espressif/esp_system.h"
#include "config.h"
//
char *replaceWord(const char *s, const char *oldW, 
                                 const char *newW);
//
// static const char *  get_my_id(void);                                 
static const char *  get_my_id(void)
{
    // Use MAC address for Station as unique ID
    static char my_id[13];
    static bool my_id_done = false;
    int8_t i;
    uint8_t x;
    if (my_id_done)
        return my_id;
    if (!sdk_wifi_get_macaddr(STATION_IF, (uint8_t *)my_id))
        return NULL;
    for (i = 5; i >= 0; --i)
    {
        x = my_id[i] & 0x0F;
        if (x > 9) x += 7;
        my_id[i * 2 + 1] = x + '0';
        x = my_id[i] >> 4;
        if (x > 9) x += 7;
        my_id[i * 2] = x + '0';
    }
    my_id[12] = '\0';
    my_id_done = true;
    return my_id;
}
#endif