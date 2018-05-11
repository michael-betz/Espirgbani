#ifndef WIFI_STARTUP_H
#define WIFI_STARTUP_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "libesphttpd/esp.h"
#include "libesphttpd/httpd.h"
#include "cJSON.h"
#include "lwip/inet.h"


#define SETTINGS_FILE	"/SD/settings.json"
#define N_WIFI_TRYS		6
#define N_HOSPOT_LOOPS  50
#define WIFI_DELAY		10000	//[ms]
#define DNS_TIMEOUT 	10000 	//[ms]

#define GET_HOSTNAME() jGetSD(getSettings(),"hostname","espirgbani")

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

#define jGet( a,b)  cJSON_GetObjectItemCaseSensitive(a,b)
#define jGetI(a,b) (jGet(a,b) ? jGet(a,b)->valueint : 0)
#define jGetD(a,b) (jGet(a,b) ? jGet(a,b)->valuedouble : 0)
#define jGetS(a,b) (jGet(a,b) ? jGet(a,b)->valuestring : 0)
// Returns a fallback string on json error
const char *jGetSD( const cJSON *j, const char *sName, const char *sDefault );

// Returns the `settings.json` singleton object
cJSON *getSettings();
// Reloads the settings singleton
void reloadSettings();
// The webserver callbakc
CgiStatus cgiReloadSettings(HttpdConnData *connData);

// Resolve a hostname
ip4_addr_t dnsResolve( const char *dnsRequestBuffer );

// Ping a host. Blocks. Returns 1 on success.
uint32_t isPingOk( ip4_addr_t *ip, uint32_t timeout_ms );

#endif
