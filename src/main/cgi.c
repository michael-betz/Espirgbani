//--------------------------------------------------------
// esp32Httpd cgi callback for serving files from SPIFFS
//--------------------------------------------------------

#include <errno.h>
#include <dirent.h> 
#include <sys/stat.h>
// #include <sys/statvfs.h>
#include "libesphttpd/esp.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "cJSON.h"

#include "web_console.h"
#include "app_main.h"
#include "cgi.h"

static const char *T = "CGI_C";

//-----------------------------------------
// List spiffs files as json
//-----------------------------------------
CgiStatus ICACHE_FLASH_ATTR cgiEspFilesListHook(HttpdConnData *connData) {
	struct stat st;
	struct dirent *dir;
	const char* prefix = (const char*)connData->cgiArg;
	DIR *d;
	cJSON *jRoot, *jFiles, *jFile;
	size_t partTotal = 0, partUsed = 0;
	// int maxFiles = 32;
	char *jsonString, lastFname[35], tempBuff[37];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}
	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "application/json");
	// httpdHeader(connData, "Cache-Control", "max-age=10, must-revalidate");
	httpdEndHeaders(connData);

	if (NULL == (d=opendir(prefix))) {
		ESP_LOGE(T,"Could not open root folder: %s", strerror(errno) );
		return HTTPD_CGI_DONE;
	}
	jRoot = cJSON_CreateObject();
	if (strcmp(prefix,"/S") == 0){
		esp_spiffs_info(NULL, &partTotal, &partUsed);
		cJSON_AddNumberToObject( jRoot, "spiffsTotal", partTotal );
		cJSON_AddNumberToObject( jRoot, "spiffsUsed",  partUsed );
	}
	// struct statvfs fsStats;
	// if( statvfs( prefix, &fsStats) >= 0 ){
	// 	cJSON_AddNumberToObject( jRoot, "fsTotal", fsStats.f_frsize*fsStats.f_blocks );
	// 	cJSON_AddNumberToObject( jRoot, "fsFree",  fsStats.f_bsize *fsStats.f_bfree  );
	// }
	cJSON_AddItemToObject(   jRoot, "files", 	 jFiles=cJSON_CreateObject() );
	while ( (dir = readdir(d)) ) {
		strcpy( lastFname, dir->d_name );
		cJSON_AddItemToObject( jFiles, dir->d_name, jFile=cJSON_CreateObject() );
		cJSON_AddNumberToObject( jFile, "d_type", dir->d_type );
		snprintf( tempBuff, 37, "%s/%s", prefix, dir->d_name );
		if( stat(tempBuff, &st) >= 0 ){
			cJSON_AddNumberToObject( jFile, "st_size",  st.st_size );
			cJSON_AddNumberToObject( jFile, "st_atime", st.st_atime );
			cJSON_AddNumberToObject( jFile, "st_mtime", st.st_mtime );
			cJSON_AddNumberToObject( jFile, "st_ctime", st.st_ctime );
		} else {
			ESP_LOGE(T,"stat( %s ) failed: %s", tempBuff, strerror(errno) );
		}
		ESP_LOGW(T, "%s (%d)\n", dir->d_name, dir->d_type );
	}
	closedir(d);
	jsonString = cJSON_Print( jRoot );
	httpdSend(connData, jsonString, strlen(jsonString) );
	free( jsonString );
	cJSON_Delete( jRoot );
	return HTTPD_CGI_DONE;
}

//-----------------------------------------
// Transfer file to / from vfs
//-----------------------------------------
//This is a catch-all cgi function. It takes the url passed to it, looks up the corresponding
//path in the vfs filesystem and if it exists, passes the file through. This simulates what a normal
//webserver would do with static files.
// Exa,ple file transfer to SD card:  curl -X POST http://Espirgbani/SD/test.html --data-binary "@html/debug/index.html"
CgiStatus ICACHE_FLASH_ATTR cgiEspVfsHook(HttpdConnData *connData) {
	FILE *file=connData->cgiData;
	HttpdPostData *p;
	int len;
	char buff[1024];
	const char *rwMode, *fName;

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		fclose(file);
		return HTTPD_CGI_DONE;
	}

	//-----------------------------------------
	// First call
	//-----------------------------------------
	// Initialize state variables
	if (file==NULL) {
		rwMode = (connData->requestType==HTTPD_METHOD_POST) ? "w" : "r";
		//Open a different file than provided in http request.
		//Common usage: {"/", cgiEspFsHook, "/index.html"} will show content of index.html without actual redirect to that file if host root was requested
		fName  = (connData->cgiArg) ? (char*)connData->cgiArg : connData->url;
		file = fopen( fName, rwMode );
		ESP_LOGI(T, "%d = fopen(%s, %s)", (int)file, fName, rwMode );
		if (file==NULL) {
			return HTTPD_CGI_NOTFOUND;
		}
		connData->cgiData=file;
		if( connData->requestType==HTTPD_METHOD_GET ){
			httpdStartResponse(connData, 200);
			httpdHeader(connData, "Content-Type", httpdGetMimetype(connData->url));
			httpdHeader(connData, "Cache-Control", "max-age=3600, must-revalidate");
			httpdEndHeaders(connData);
			return HTTPD_CGI_MORE;
		} else {
			ESP_LOGI(T, "Writing %d bytes", connData->post->len );
		}
	}

	if( connData->requestType==HTTPD_METHOD_GET ){
		//-----------------------------------------
		// GET /*
		//-----------------------------------------
		// Read file from vfs
		len=fread(buff, 1, 1024, file);
		if (len>0) httpdSend(connData, buff, len);
		if (len!=1024) {
			//We're done.
			fclose(file);
			return HTTPD_CGI_DONE;
		} else {
			//Ok, till next time.
			return HTTPD_CGI_MORE;
		}
	} else {
		//----------------------------------------------------
		// POST /*
		//----------------------------------------------------
		// (Over)write file on vfs
		p = connData->post;
		len = fwrite( p->buff, 1, p->buffLen, file );
		if ( len != p->buffLen ){
			fclose(file);
			ESP_LOGE(T, "%d = fwrite(), should be %d, err: %s", len, p->buffLen, strerror(errno) );
			httpdStartResponse(connData, 400);
			httpdHeader(connData, "Content-Type", "text/plain");
			httpdEndHeaders(connData);
			httpdSend(connData, strerror(errno), -1);
			return HTTPD_CGI_DONE;
		}
		if (connData->post->len==connData->post->received) {
			//We're done! Format a response.
			fclose(file);
			ESP_LOGI(T, "Upload done.\n");
			httpdStartResponse(connData, 200);
			httpdHeader(connData, "Content-Type", "text/plain");
			httpdEndHeaders(connData);
			return HTTPD_CGI_DONE;
		} else {
			//Ok, till next time.
			return HTTPD_CGI_MORE;
		}	
	}
}

typedef struct {
	int state;
	char *rPtr;
	int nBytesLeft;
}cgiRtcLogState_t;
//-----------------------------------------
// GET the log-buffer from RTC-mem
//-----------------------------------------
CgiStatus ICACHE_FLASH_ATTR cgiEspRTC_LOG(HttpdConnData *connData) {
	cgiRtcLogState_t* state = (cgiRtcLogState_t*)connData->cgiData;
	int nSend;
	static const char *logBuffEnd = rtcLogBuffer + LOG_FILE_SIZE - 1;
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}
	// Initialize state variables
	if ( state == NULL ) {
		state = malloc( sizeof(cgiRtcLogState_t) );
		state->state = 0;
		//-----------------------------------------
		// First call, send oldest to end
		//-----------------------------------------
		state->rPtr = rtcLogWritePtr;
		state->nBytesLeft = logBuffEnd-rtcLogWritePtr+1;
		connData->cgiData = state;
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "content-type", "text/plain");
		httpdEndHeaders(connData);
	} 
	nSend = MIN( state->nBytesLeft, 1024 );
	httpdSend( connData, state->rPtr, nSend );
	// printf("state %d, sending:", state->state);
	// hexDump( (uint8_t*)state->rPtr, nSend );
	state->rPtr += nSend;
	state->nBytesLeft -= nSend;
	if( state->nBytesLeft > 0 ){
		return HTTPD_CGI_MORE;
	} else {
		// done
		switch( state->state ){
		case 0:
		//-----------------------------------------
		// Second call, send beginning to newest
		//-----------------------------------------		
		// rtcLogWritePtr is on the oldest entry. Send [:newest]
			state->rPtr = rtcLogBuffer;
			state->nBytesLeft = rtcLogWritePtr-rtcLogBuffer;
			state->state++;
			return HTTPD_CGI_MORE;
		case 1:
		default:
			free( state );
			state = NULL;
			return HTTPD_CGI_DONE;
		}
	}
}