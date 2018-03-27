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
#include "esp_ping.h"
#include "ping.h"
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
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/dns.h"

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
    uint8_t *rip = ws->conn->remote_ip;
    ESP_LOGI(T,"WS-client: %d.%d.%d.%d", rip[0], rip[1], rip[2], rip[3] );
    ws->recvCb = wsReceive;
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
    {"/reload",        cgiReloadSettings,       NULL },
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
            tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, GET_HOSTNAME());
            tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_AP,  GET_HOSTNAME());
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
        ESP_LOGE(T,"JSON Error in %s", fName );
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(T, "Error before: %s", error_ptr);
        }
    }
    free( txtData );
    return root;
}

cJSON *g_settings = NULL;
cJSON *getSettings(){
    if (!g_settings) {
        g_settings = readJsonDyn( SETTINGS_FILE );
        if( g_settings ){
            ESP_LOGI(T, "Loaded %s:", SETTINGS_FILE);
        } else {
            ESP_LOGE(T, "Error loading %s", SETTINGS_FILE);
        }
    }
    return g_settings;
}

const char *jGetSD( const cJSON *j, const char *sName, const char *sDefault ){
    const cJSON *jTemp = cJSON_GetObjectItemCaseSensitive( j, sName );
    if( !cJSON_IsString(jTemp) ){
        ESP_LOGE(T, "json error: %s is not a string", sName);
        return sDefault;
    }
    return jTemp->valuestring;
}

void reloadSettings(){
    if (g_settings) {
        cJSON_Delete(g_settings);
        g_settings = NULL;
    }
}

// Handle request to reload settings
CgiStatus ICACHE_FLASH_ATTR cgiReloadSettings(HttpdConnData *connData) {
    if (connData->conn==NULL) {
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }
    reloadSettings();
    httpdStartResponse(connData, 200);
    httpdHeader(connData, "Content-Type", "text/plain");
    httpdEndHeaders(connData);
    char *str = cJSON_Print(getSettings());
    httpdSend(connData, str, strlen(str));
    free(str);
    return HTTPD_CGI_DONE;
}

int getKnownApPw( wifi_ap_record_t *foundAps, uint16_t foundNaps, uint8_t *ssid, uint8_t *pw ){
    // Goes through wifi scan result (foundAps) and looks for a known wifi (from json)
    // If found returns 0 and copies data into *ssid and *pw
    // If no goo done is found, returns -1
    //---------------------------------------------------
    // See if there are any known wifis
    //---------------------------------------------------
    // Index json file by key (ssid)
    const cJSON *jWifis=NULL, *jWifi=NULL;
    wifi_ap_record_t *currAp=NULL;
    int i;
    ESP_LOGI(T, "\n------------------ Found %d wifis --------------------", foundNaps );
    for( i=0,currAp=foundAps; i<foundNaps; i++ ){
        ESP_LOGI(T, "ch: %2d, ssid: %16s, bssid: %02x:%02x:%02x:%02x:%02x:%02x, rssi: %d", currAp->primary, (char*)currAp->ssid, currAp->bssid[0], currAp->bssid[1], currAp->bssid[2], currAp->bssid[3], currAp->bssid[4], currAp->bssid[5], currAp->rssi );
        currAp++;
    }
    jWifis = cJSON_GetObjectItemCaseSensitive( getSettings(), "wifis");
    // Go through all found APs, use ssid as key and try to get item from json
    for( i=0,currAp=foundAps; i<foundNaps; i++ ){
        jWifi = cJSON_GetObjectItemCaseSensitive( jWifis, (char*)currAp->ssid);
        currAp++;
        if ( !cJSON_IsObject(jWifi) ){
            continue;
        }
        //---------------------------------------------------
        // Found a known good WIFI, connect to it ...
        //---------------------------------------------------
        ESP_LOGW(T,"match: %s, trying to connect ...", jWifi->string );
        strcpy( (char*)ssid, jWifi->string );
        strcpy( (char*)pw, jGetSD(jWifi,"pw","") );
        return 0;
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
        .ap.password = "",
        .ap.channel = 5,
        .ap.authmode = WIFI_AUTH_OPEN,
        .ap.max_connection = 4,
        .ap.beacon_interval = 100
    };
    strcpy( (char*)wifi_config.ap.ssid, GET_HOSTNAME() );
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

xSemaphoreHandle dnsCallbackSema = NULL;
ip_addr_t g_dnsResponse;

void dnsCallback(const char *name, const ip_addr_t *ipaddr, void *callback_arg){
    memcpy( &g_dnsResponse, ipaddr, sizeof(ip_addr_t) );
    xSemaphoreGive( dnsCallbackSema );
}

// dnsRequestBuffer = zero terminated string of domain to be dns'ed, returns 32 bit IP if good, 0 otherwise
ip4_addr_t dnsResolve( const char *dnsRequestBuffer ){
    if (dnsCallbackSema == NULL){
        vSemaphoreCreateBinary( dnsCallbackSema );
    }
    xSemaphoreTake( dnsCallbackSema, 0 );       // Make sure smea is not available already (must be given by dnsCallback)
    dns_init();
    ip_addr_t responseIp;
    dns_gethostbyname( dnsRequestBuffer, &responseIp, dnsCallback, NULL);
    if( xSemaphoreTake( dnsCallbackSema, DNS_TIMEOUT/portTICK_PERIOD_MS ) ){
        ESP_LOGD( T, "DNS for %s done. Response = %s", dnsRequestBuffer, ip4addr_ntoa(&g_dnsResponse.u_addr.ip4) );
    } else {
        ESP_LOGW( T, "DNS for %s timeout", dnsRequestBuffer);
        ip4_addr_set_any( &g_dnsResponse.u_addr.ip4 );
    }
    return g_dnsResponse.u_addr.ip4;
}

xSemaphoreHandle pingCallbackSema = NULL;
uint32_t g_pingRespTime;

esp_err_t pingResults(ping_target_id_t msgType, esp_ping_found * pf){
    switch(pf->ping_err){
        case PING_RES_FINISH:
            return ESP_OK;
        case PING_RES_TIMEOUT:
            g_pingRespTime = 0;
            xSemaphoreGive( pingCallbackSema );
            break;
        case PING_RES_OK:
            g_pingRespTime = pf->resp_time;
            xSemaphoreGive( pingCallbackSema );
            break;
    }
    ESP_LOGD(T,"RespTime:%d Sent:%d Rec:%d ErrCnt:%d Stat:%d, TimeoutCnt:%d", pf->resp_time, pf->send_count, pf->recv_count, pf->err_count, pf->ping_err, pf->timeout_count);
    return ESP_OK;
}

uint32_t isPingOk( ip4_addr_t *ip, uint32_t timeout_ms ){
    if( !ip ) return 0;
    if (pingCallbackSema == NULL){
        vSemaphoreCreateBinary( pingCallbackSema );
    }
    xSemaphoreTake( pingCallbackSema, 0 ); // Make sure smea is not available already (must be given by pingResults)
    esp_ping_set_target(PING_TARGET_IP_ADDRESS,       ip,    sizeof(uint32_t));
    uint32_t x = 1;     // 1 ping per report
    esp_ping_set_target(PING_TARGET_IP_ADDRESS_COUNT, &x,     sizeof(uint32_t));
    x = 0;              // 0 delay between pings
    esp_ping_set_target(PING_TARGET_DELAY_TIME,       &x,    sizeof(uint32_t));
    esp_ping_set_target(PING_TARGET_RCV_TIMEO,        &timeout_ms,    sizeof(uint32_t));
    esp_ping_set_target(PING_TARGET_RES_FN,           &pingResults, sizeof(pingResults));
    ping_init();
    if( xSemaphoreTake( pingCallbackSema, (timeout_ms*2)/portTICK_PERIOD_MS ) ){
        return g_pingRespTime;
    }
    return 0;
}
