#ifndef CGI_H
#define CGI_H

#include "libesphttpd/httpd.h"

CgiStatus ICACHE_FLASH_ATTR cgiEspSPIFFSHook(HttpdConnData *connData);
CgiStatus ICACHE_FLASH_ATTR cgiEspSPIFFSListHook(HttpdConnData *connData);
CgiStatus ICACHE_FLASH_ATTR cgiEspRTC_LOG(HttpdConnData *connData);

#endif