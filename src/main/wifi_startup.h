#ifndef WIFI_STARTUP_H
#define WIFI_STARTUP_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define N_WIFI_TRYS		6
#define WIFI_DELAY		10000	//[ms]
#define HOSTNAME 		"espirgbani"

#define CONNECTED_BIT 	BIT0
#define STA_START_BIT	BIT1


typedef enum {
    WIFI_CON_TO_AP_MODE,
    WIFI_CONNECTED,
    WIFI_START_HOTSPOT_MODE,
    WIFI_HOTSPOT_RUNNING,
    WIFI_OFF_MODE,
    WIFI_IDLE
}wifiState_t;

extern EventGroupHandle_t wifi_event_group;
extern wifiState_t wifiState;

extern void wifi_conn_init(void);
extern void wifi_disable();

#endif