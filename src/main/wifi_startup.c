#include <errno.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_spi_flash.h"
#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "libesphttpd/esp.h"
#include "libesphttpd/espfs.h"
#include "libesphttpd/httpdespfs.h"
#include "libesphttpd/webpages-espfs.h"
#include "libesphttpd/cgiflash.h"
#include "libesphttpd/cgiwifi.h"
#include "libesphttpd/cgiwebsocket.h"
#include "libesphttpd/captdns.h"

#include "apps/sntp/sntp.h"
#include <lwip/err.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>

#include "cgi.h"
#include "cJSON.h"

#include "app_main.h"
#include "rgb_led_panel.h"

#include "wifi_startup.h"
#include "font.h"

static const char *T = "WIFI_STARTUP";
EventGroupHandle_t wifi_event_group;
wifiState_t wifiState=WIFI_CON_TO_AP_MODE;

void wsReceive(Websock *ws, char *data, int len, int flags){
    uint16_t b = 0;
    ESP_LOGI(T, "wsrx:");
    ESP_LOG_BUFFER_HEXDUMP(T, data, len, ESP_LOG_INFO);
    switch( data[0] ){
        case WS_ID_BRIGHTNESS:
            // Set brightness level
            b = atoi( &data[1] );
            if( b>1 && b<120 ){
                switch( brightNessState ){
                    case BR_DAY:
                        ESP_LOGI(T, "dayBrightness = %d", b);
                        dayBrightness = b;
                        break;
                    case BR_NIGHT:
                        ESP_LOGI(T, "nightBrightness = %d", b);
                        nightBrightness = b;
                        break;
                }
                g_rgbLedBrightness = b;
            }
            break;
        
        case WS_ID_LOGLEVEL:
            // Set log level
            if( len != 2 )  return;
            switch( data[1] ){
                case 'E':   esp_log_level_set("*", ESP_LOG_ERROR);   break;
                case 'W':   esp_log_level_set("*", ESP_LOG_WARN);    break;
                case 'I':   esp_log_level_set("*", ESP_LOG_INFO);    break;
                case 'D':   esp_log_level_set("*", ESP_LOG_DEBUG);   break;
                case 'V':   esp_log_level_set("*", ESP_LOG_VERBOSE); break;
            }
            break;

        case WS_ID_FONT:
            // Set font type (quick and dirty)
            b = atoi( &data[1] );
            if( b<g_maxFnt ){
                g_fontNumberRequest = b;
            }
            break;
    }
}

// Websocket connected.
static void wsConnect(Websock *ws) {
    char tempBuff[32];
    int b;
    uint8_t *rip = ws->conn->remote_ip;
    ESP_LOGI(T,"WS-client: %d.%d.%d.%d", rip[0], rip[1], rip[2], rip[3] );

    ws->recvCb = wsReceive;
    switch( brightNessState ){
        case BR_DAY:
            b = dayBrightness;
            break;
        case BR_NIGHT:
            b = nightBrightness;
            break;
        default:
            b = 0;
    }
    sprintf( tempBuff, "%c%d", WS_ID_BRIGHTNESS, b );
    cgiWebsocketSend( ws, tempBuff, strlen(tempBuff), WEBSOCK_FLAG_NONE );
    sprintf( tempBuff, "%c%d", WS_ID_MAXFONT, g_maxFnt );
    cgiWebsocketSend( ws, tempBuff, strlen(tempBuff), WEBSOCK_FLAG_NONE );
}

//------------------------------
// Setup webserver
//------------------------------
// Flash config
CgiUploadFlashDef uploadParams = {
    .type=CGIFLASH_TYPE_FW,
    .fw1Pos=0x010000,
    .fw2Pos=0x0D0000,
    .fwSize=0x0C0000,
    .tagName="main1"
};
// Webserver config
const HttpdBuiltInUrl builtInUrls[]={
    // {"*", cgiRedirectApClientToHostname, HOSTNAME},
    {"/", cgiRedirect, "/index.html"},
    {"/ws.cgi",        cgiWebsocket,            wsConnect },
    {"/debug",         cgiRedirect,             "/debug/index.html" },
    {"/flash",         cgiRedirect,             "/flash/index.html" },
    {"/flash/upload",  cgiUploadFirmware,       &uploadParams },
    {"/reboot",        cgiRebootFirmware,       NULL },
    {"/log.txt",       cgiEspRTC_LOG,           NULL },
    {"/S",             cgiEspFilesListHook,    "/S" },
    {"/SD",            cgiEspFilesListHook,    "/SD"},
    {"/S/*",           cgiEspVfsHook,           NULL }, //Catch-all cgi function for the SPIFFS filesystem
    {"/SD/*",          cgiEspVfsHook,           NULL }, //Catch-all cgi function for the SD card filesystem
    {"*",              cgiEspFsHook,            NULL }, //Catch-all cgi function for the static filesystem
    {NULL, NULL, NULL}
};

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{  
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, HOSTNAME);
            tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_AP,  HOSTNAME);
            tcpip_adapter_dns_info_t dnsIp;
            ipaddr_aton("192.168.4.1", &dnsIp.ip );
            ESP_ERROR_CHECK( tcpip_adapter_set_dns_info(TCPIP_ADAPTER_IF_AP, TCPIP_ADAPTER_DNS_MAIN, &dnsIp) );
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            if (!sntp_enabled()){
                ESP_LOGI(T, "Initializing SNTP");
                sntp_setoperatingmode(SNTP_OPMODE_POLL);
                sntp_setservername(0, "pool.ntp.org");
                sntp_init();
            }
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            /* This is a workaround as ESP32 WiFi libs don't currently
               auto-reassociate. */
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

char *readFileDyn( char* fName, int *fSize ){
    // opens the file fName and returns it as dynamically allocated char*
    // if fSize is not NULL, copies the number of bytes read there
    // dont forget to fall free() on the char* result
    FILE *f = fopen(fName, "rb");
    if ( !f ) {
        ESP_LOGE( T, " fopen( %s ) failed: %s", fName, strerror(errno) );
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);
    // ESP_LOGI(T, "filesize: %d", fsize );
    char *string = malloc( fsize + 1 );
    if ( !string ) {
        ESP_LOGE( T, "malloc( %d ) failed: %s", fsize+1, strerror(errno) );
        return NULL;
    }
    fread(string, fsize, 1, f);
    fclose(f);
    string[fsize] = 0;
    if( fSize )
        *fSize = fsize;
    return string;
}

cJSON *readJsonDyn( char* fName ){
    // opens the json file `fName` and returns it as cJSON*
    // don't forget to call cJSON_Delete() on it
    cJSON *root;
    char *txtData = readFileDyn( fName, NULL );
    if( !txtData ){
        return NULL;
    }
    if( !(root = cJSON_Parse( txtData )) ){
        ESP_LOGE(T,"No data in %s", fName );
    }
    free( txtData );
    return root;
}

int getKnownApPw( wifi_ap_record_t *foundAps, uint16_t foundNaps, uint8_t *ssid, uint8_t *pw ){
    // Goes through wifi scan result (foundAps) and looks for a known wifi (from json)
    // If found returns 0 and copies data into *ssid and *pw
    // If no goo done is found, returns -1
    //---------------------------------------------------
    // See if there are any known wifis
    //---------------------------------------------------
    // Index json file by key (ssid)
    cJSON *jRoot=NULL, *jWifi=NULL, *jTemp=NULL;
    wifi_ap_record_t *currAp=NULL;
    int i;
    ESP_LOGI(T, "\n------------------ Found %d wifis --------------------", foundNaps );
    for( i=0,currAp=foundAps; i<foundNaps; i++ ){
        ESP_LOGI(T, "ch: %2d, ssid: %16s, bssid: %02x:%02x:%02x:%02x:%02x:%02x, rssi: %d", currAp->primary, (char*)currAp->ssid, currAp->bssid[0], currAp->bssid[1], currAp->bssid[2], currAp->bssid[3], currAp->bssid[4], currAp->bssid[5], currAp->rssi );
        currAp++;
    }
    if( (jRoot = readJsonDyn("/SD/knownWifis.json")) ){
        // Go through all found APs, use ssid as key and try to get item from json
        for( i=0,currAp=foundAps; i<foundNaps; i++ ){
            jWifi = cJSON_GetObjectItem( jRoot, (char*)currAp->ssid);
            currAp++;
            if ( !jWifi ){
                continue;
            }
            if( !(jTemp = cJSON_GetObjectItem( jWifi, "type" )) || (jTemp->type!=cJSON_String) ){
                ESP_LOGE(T, "Wifi type undefined");
                continue;
            }
            if( strcmp(jTemp->valuestring, "full") != 0 ){
                ESP_LOGW(T, "Skipping %s due to unsupported Wifi type %s", currAp->ssid, jTemp->valuestring );
                continue;
            }
            //---------------------------------------------------
            // Found a known good WIFI, connect to it ...
            //---------------------------------------------------
            ESP_LOGW(T,"match: %s, trying to connect ...", jWifi->string );
            strcpy( (char*)ssid, jWifi->string );
            if( !(jTemp = cJSON_GetObjectItem( jWifi, "pw" )) || (jTemp->type!=cJSON_String) ){
                ESP_LOGE(T, "Wifi pw undefined");
                strcpy( (char*)pw,   "" );
            } else {
                strcpy( (char*)pw,   jTemp->valuestring );
            }
            cJSON_Delete( jRoot ); jRoot = NULL;
            return 0;
        }
        cJSON_Delete( jRoot ); jRoot = NULL;
    }
    return -1;
}

void conToApMode(){
    // goes through all steps needed to connect to a known wifi.
    // if succesfuls, sets wifiState to WIFI_CONNECTED
    int ret=0;
    uint16_t foundNaps=0;
    wifi_ap_record_t* foundAps = NULL;
    wifi_config_t wifi_config = {
        .sta.scan_method = WIFI_ALL_CHANNEL_SCAN,
        .sta.bssid_set = 0,
        .sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL
    };
    ESP_ERROR_CHECK( esp_wifi_stop() );
    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
    ESP_ERROR_CHECK( esp_wifi_set_config( ESP_IF_WIFI_STA, &wifi_config ) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    //---------------------------------------------------
    // Do a wifi scan and see what is around
    //---------------------------------------------------
    wifi_scan_config_t scanConfig = {
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE
    };
    ESP_ERROR_CHECK( esp_wifi_scan_start( &scanConfig, 1 ) );
    // ... bLocks for 1500 ms ...
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_num( &foundNaps ) );
    // copy AP data in local memory
    assert( (foundAps = (wifi_ap_record_t*) malloc( sizeof(wifi_ap_record_t)*foundNaps )) );
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_records( &foundNaps, foundAps ) );
    // Compare scan result against known wifis from .json file
    ret = getKnownApPw( foundAps, foundNaps, wifi_config.sta.ssid, wifi_config.sta.password );
    free( foundAps ); foundAps = NULL;
    if( ret < 0 ){
        // no useful APs found
        return;
    }
    ESP_ERROR_CHECK( esp_wifi_stop() );
    ESP_ERROR_CHECK( esp_wifi_set_config( ESP_IF_WIFI_STA, &wifi_config ) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    // Wait up to 10 s for an IP
    xEventGroupWaitBits( wifi_event_group, CONNECTED_BIT, pdFALSE, pdTRUE, 10000/portTICK_PERIOD_MS );
    // Check if we are connected
    if( xEventGroupGetBits( wifi_event_group ) & CONNECTED_BIT ){
        ESP_LOGI(T,"Conected to an AP. All is good :)");
        wifiState = WIFI_CONNECTED;
    }
}

void startHotspotMode(){
    wifi_config_t wifi_config = {
        .ap.ssid = HOSTNAME,
        .ap.password = "",
        .ap.channel = 5,
        .ap.authmode = WIFI_AUTH_OPEN,
        .ap.max_connection = 4,
        .ap.beacon_interval = 100
    };
    ESP_LOGI(T, "WIFI_HOTSPOT_MODE");
    ESP_ERROR_CHECK( esp_wifi_stop() );
    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_AP ) );
    ESP_ERROR_CHECK( esp_wifi_set_config( ESP_IF_WIFI_AP, &wifi_config ) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void wifiConnectionTask(void *pvParameters){
    int wifiTry=0;   
    wifi_sta_list_t hsStas;

    //------------------------------
    // Start http server
    //------------------------------
    ESP_ERROR_CHECK( espFsInit((void*)(webpages_espfs_start)) );
    ESP_ERROR_CHECK( httpdInit(builtInUrls, 80, 0) );
    ESP_LOGW(T,"FW 1, RAM left %d\n", esp_get_free_heap_size() );
    captdnsInit();

    while(1){
        switch( wifiState ){
            case WIFI_CON_TO_AP_MODE:
                //---------------------------------------------------
                // Connect to another access point as client
                //---------------------------------------------------
                conToApMode();
                if( ++wifiTry >= N_WIFI_TRYS ){
                    ESP_LOGW(T,"No known Wifis found, switching to hotspot mode");
                    wifiState = WIFI_START_HOTSPOT_MODE;
                }    
                break;

            case WIFI_CONNECTED:
                //---------------------------------------------------
                // Connected and happy, do nothing
                //---------------------------------------------------
                if( xEventGroupGetBits( wifi_event_group ) & CONNECTED_BIT ){
                    vTaskDelay( 1000/portTICK_PERIOD_MS );
                } else {
                    ESP_LOGW(T, "Wifi connection lost !!! reconnecting ...");
                    wifiState = WIFI_CON_TO_AP_MODE;
                }
                break;

            case WIFI_START_HOTSPOT_MODE:
                //---------------------------------------------------
                // Start an soft access point
                //---------------------------------------------------
                startHotspotMode();
                wifiState = WIFI_HOTSPOT_RUNNING;
                wifiTry = 0;
                break;
            
            case WIFI_HOTSPOT_RUNNING:
                //---------------------------------------------------
                // Run hotspot. Timeout when noone is connected
                //---------------------------------------------------
                if( esp_wifi_ap_get_sta_list( &hsStas ) == ESP_OK ){
                    if ( hsStas.num == 0 ){
                        if( ++wifiTry > N_WIFI_TRYS ){
                            wifiState = WIFI_OFF_MODE;
                            continue;
                        }
                        ESP_LOGW(T, "Hotspot timeout: %d", wifiTry);
                    } else {
                        wifiTry = 0;
                    }
                }
                vTaskDelay( WIFI_DELAY/portTICK_PERIOD_MS );
                break;
                
            case WIFI_OFF_MODE:
                //---------------------------------------------------
                // Disconnect wifi
                //---------------------------------------------------
                wifi_disable();
                ESP_ERROR_CHECK( esp_wifi_stop() );
                ESP_LOGI(T, "WIFI_OFF_MODE");
                wifiState = WIFI_IDLE;
                wifiTry = 0;
                break;

            case WIFI_IDLE:
            default:
                //---------------------------------------------------
                // Do nothing (wheel ticks should keep it alive)
                //---------------------------------------------------
                // if( ++wifiTry > N_WIFI_TRYS ){
                //     ESP_LOGW(T, "Going to deep sleep ...");
                //     wifiTry = 0;
                //     // go to deep sleep
                //     // rtc_gpio_pullup_dis( GPIO_SPEED_PLS );
                //     // rtc_gpio_pulldown_dis( GPIO_SPEED_PLS );
                //     rtc_gpio_pullup_en( GPIO_SPEED_PLS );
                //     ESP_ERROR_CHECK( esp_sleep_enable_ext0_wakeup( GPIO_SPEED_PLS, 0 ) );
                //     esp_deep_sleep_start();
                // }
                vTaskDelay( WIFI_DELAY/portTICK_PERIOD_MS );
                break;
        } // switch
    } // while(1)
    vTaskDelete( NULL );
}

void wifi_conn_init(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    xTaskCreate(&wifiConnectionTask, "wifiConnectionTask", 4096, NULL, 5, NULL);
}

void wifi_disable(){
    esp_wifi_stop();
    // All done, unmount partition and disable SPIFFS
    esp_vfs_spiffs_unregister(NULL);
}