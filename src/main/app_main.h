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
#define RAND_AB(a,b) (rand()%(b+1-a)+a)

enum {BR_DAY, BR_ALONE, BR_NIGHT} brightNessState;
extern int g_maxFnt;	//Max number prefix of the font filename
extern int g_fontNumberRequest;

#endif