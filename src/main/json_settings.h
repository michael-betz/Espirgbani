#ifndef JSON_SETTINGS_H
#define JSON_SETTINGS_H

#define jGet(a, b) (cJSON_GetObjectItemCaseSensitive(a, b))
#define jGetI(a,b) (jGet(a,b) ? jGet(a,b)->valueint : 0)
#define jGetD(a,b) (jGet(a,b) ? jGet(a,b)->valuedouble : 0)
#define jGetS(a,b) (jGet(a,b) ? jGet(a,b)->valuestring : 0)

// Returns a fallback string on json error
const char *jGetSD( const cJSON *j, const char *sName, const char *sDefault );

// Returns the `settings.json` singleton object
cJSON *getSettings();

// Reloads the settings singleton
void reloadSettings();

#endif
