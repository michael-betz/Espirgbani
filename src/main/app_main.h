#ifndef APP_MAIN_H
#define APP_MAIN_H

#define WS_ID_WEBCONSOLE 'a'

#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a>b?a:b)
#define pi M_PI

// Random number within the range [a,b]
#define RAND_AB(a,b) (rand()%(b+1-a)+a)

extern int dayBrightness;
extern int nightBrightness;
enum {BR_DAY, BR_NIGHT} brightNessState;

#endif