
#include <stdio.h>
#include <errno.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "libesphttpd/esp.h"
#include "libesphttpd/cgiwebsocket.h"
#include "web_console.h"
#include "app_main.h"

static const char *T = "WEBCONSOLE";

RTC_DATA_ATTR char rtcLogBuffer[LOG_FILE_SIZE];
RTC_DATA_ATTR char *rtcLogWritePtr = rtcLogBuffer;

// // writes string to spiffs in /S/a.txt
// // if a.txt > LOG_FILE_SIZE the rename it to b.txt
// // (overwrite it if it already exists)
// // Warning SPIFFS can slow down the whole ESP, if it's full
// // It's propably not a good idea to abuse the flash memory like that
// void writeToLogFile( const char* str ){
//     FILE *f = NULL;
//     struct stat st;
//     int ret;
//     if( f == NULL ){
//         if( (f = fopen("/S/a.txt", "a")) == NULL ){
//             // ESP_LOGE(T, "Open /a.txt failed: %s", strerror(errno) );
//             goto writeToLogFileFinally;
//         }
//     }
//     if( (ret=fwrite(str, 1, strlen(str), f)) <= 0 ){
//         // ESP_LOGE(T, "Write failed: %s", strerror(errno) );
//         goto writeToLogFileFinally;
//     }
//     fflush( f );
//     // ESP_LOGI(T,"Wrote %d bytes", ret);
    
//     // Get size of /a.txt
//     if ( stat("/S/a.txt", &st) == 0 ) {
//         // Check if a.txt is over the size limit
//         // ESP_LOGI(T, "size of a.txt: %d bytes", (int)st.st_size);
//         if( st.st_size > LOG_FILE_SIZE ){
//             // ESP_LOGI(T, "renaming a.txt to b.txt");
//             fclose( f );
//             f = NULL;
//             // delete b.txt if it exists
//             unlink("/S/b.txt");
//             // rename a.txt to b.txt
//             rename("/S/a.txt", "/S/b.txt");
//         }
//     }
// writeToLogFileFinally:
//     fclose( f );
//     f = NULL;
// }

// writes string to rolling log-buffer in RTC memory
void writeToLogRTC( const char* str ){
    static const char *logBuffEnd = rtcLogBuffer + LOG_FILE_SIZE - 1;
    while( *str ){
        *rtcLogWritePtr = *str++;
        if ( rtcLogWritePtr >= logBuffEnd ){
            rtcLogWritePtr = rtcLogBuffer;
        } else {
            rtcLogWritePtr++;
        }
    }
}

int wsDebugPrintf( const char *format, va_list arg ){
    static char charBuffer[512];
    static  int charLen;
    charBuffer[0] = WS_ID_WEBCONSOLE;
    charLen = vsprintf( &charBuffer[1], format, arg );
    if( charLen <= 0 ){
        return 0;
    }
    charBuffer[511] = '\0';
    cgiWebsockBroadcast("/ws.cgi", charBuffer, charLen+1, WEBSOCK_FLAG_NONE);
    // Output to UART as well
    printf( "%s", &charBuffer[1] );
    // // Output to logfile in SPIFFS
    // writeToLogFile( charBuffer );
    // Output to logbuffer in RTC mem
    writeToLogRTC( &charBuffer[1] );
    return charLen;
}

void initSpiffs(){
    int temp=0;
    ESP_LOGI(T, "Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/S",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    if ( ESP_OK != (temp=esp_vfs_spiffs_register(&conf)) ) {
        ESP_LOGE(T, "spiffs error: %d", temp);
        return;
    }
    size_t total = 0, used = 0;
    esp_spiffs_info(NULL, &total, &used);
    ESP_LOGI(T, "Partition size: total: %d, used: %d", total, used);
}

void printFile( const char *fName ){
    int c;
    FILE *file;
    ESP_LOGI(T,"Contents of file %s:", fName);
    if ((file = fopen(fName, "r"))==NULL) {
        ESP_LOGE(T, "Open failed: %s", strerror(errno) );
        return;
    }
    while ((c = getc(file)) != EOF)
        putchar( c );
    fclose(file);
}