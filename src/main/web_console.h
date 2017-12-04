#ifndef WCONSOLE_H
#define WCONSOLE_H
#include "esp_attr.h"

#define WEBCON_MODE 0	// 0 = spiffs (/S/a.txt /s/b.txt)  1 = RTC mem

// Rolling buffer size for log entries in bytes
#define LOG_FILE_SIZE (4000)

// Initialize and mount SPIFFS (format if needed)
// run at boot time
extern void initSpiffs();

// register this with esp_log_set_vprintf()
// will route log output to:
//   * websocket
//   * alternating log files on spiffs
//   * uart
extern int wsDebugPrintf( const char *format, va_list arg );

// print a file on spiffs to uart
extern void printFile( const char *fName );

// Rolling string buffer in RTC mem (survives sleep)
extern RTC_DATA_ATTR char rtcLogBuffer[LOG_FILE_SIZE];
extern RTC_DATA_ATTR char *rtcLogWritePtr;	// Points to oldest element in ring buffer

#endif