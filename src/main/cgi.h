#ifndef CGI_H
#define CGI_H

#include "libesphttpd/httpd.h"

CgiStatus ICACHE_FLASH_ATTR cgiEspVfsHook(HttpdConnData *connData);
CgiStatus ICACHE_FLASH_ATTR cgiEspFilesListHook(HttpdConnData *connData);
CgiStatus ICACHE_FLASH_ATTR cgiEspRTC_LOG(HttpdConnData *connData);

#endif