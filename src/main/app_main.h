#ifndef APP_MAIN_H
#define APP_MAIN_H

#define WS_ID_WEBCONSOLE 'a'
#define WS_ID_LOGLEVEL   'l'
#define WS_ID_FONT       'f'
#define WS_ID_MAXFONT    'g'


#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a>b?a:b)
#define pi M_PI

// Random number within the range [a,b]
#include "esp_system.h"
#define RAND_AB(a,b) (esp_random()%(b+1-a)+a)

enum {BR_DAY, BR_ALONE, BR_NIGHT} brightNessState;
extern int g_maxFnt;	//Max number prefix of the font filename
extern int g_fontNumberRequest;

#endif
