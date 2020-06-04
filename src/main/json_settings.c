#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include "cJSON.h"
#include "esp_log.h"
#include "json_settings.h"

static const char *T = "JSON_SETTINGS";

// TODO move default_settings to spiffs
static const char default_settings[] = "\n\
{\n\
    \"panel\": {\n\
        \"test_pattern\": true,\n\
        \"tp_brightness\":  10,\n\
        \"is_clk_inverted\": true,\n\
        \"column_swap\": false,\n\
        \"latch_offset\": 0,\n\
        \"extra_blank\": 1,\n\
        \"clkm_div_num\": 4\n\
    },\n\
    \"power\": {\n\
        \"hi\": {\n\
            \"h\": 9,\n\
            \"m\": 0,\n\
            \"p\": 50,\n\
            \"pingIp\": \"0.0.0.0\"\n\
        },\n\
        \"lo\": {\n\
            \"h\": 22,\n\
            \"m\": 45,\n\
            \"p\": 1\n\
        }\n\
    },\n\
    \"delays\": {\n\
        \"font\":  60,\n\
        \"color\": 10,\n\
        \"ani\":   15,\n\
        \"ping\":  60\n\
    },\n\
    \"log_level\": \"W\",\n\
    \"timezone\": \"PST8PDT,M3.2.0,M11.1.0\",\n\
    \"hostname\": \"espirgbani\",\n\
    \"wifis\": {\n\
        \"<wifi-ssid>\": {\n\
            \"pw\": \"<wifi-password>\"\n\
        }\n\
    }\n\
}\n";

char *readFileDyn(const char* file_name, int *file_size) {
    // opens the file file_name and returns it as dynamically allocated char*
    // if file_size is not NULL, copies the number of bytes read there
    // dont forget to fall free() on the char* result
    FILE *f = fopen(file_name, "rb");
    if (!f) {
        ESP_LOGE(T, " fopen(%s, rb) failed: %s", file_name, strerror(errno));
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);
    ESP_LOGD(T, "loading %s, fsize: %d", file_name, fsize);
    char *string = malloc(fsize + 1);
    if (!string) {
        ESP_LOGE(T, "malloc(%d) failed: %s", fsize + 1, strerror(errno));
        assert(false);
    }
    fread(string, fsize, 1, f);
    fclose(f);
    string[fsize] = 0;
    if (file_size)
        *file_size = fsize;
    return string;
}

cJSON *readJsonDyn(const char* file_name, bool create_file_on_error) {
    // opens the json file `file_name` and returns it as cJSON*
    // don't forget to call cJSON_Delete() on it
    cJSON *root;

    // try to read settings.json from SD card
    char *txtData = readFileDyn(file_name, NULL);

    // if it does not exist, try to create it with the default settings
    if (!txtData) {
        if (create_file_on_error) {
            ESP_LOGW(T, "writing default-settings to %s", file_name);
            FILE *f = fopen(file_name, "wb");
            if (f) {
                fputs(default_settings, f);
                fclose(f);
            } else {
                ESP_LOGE(T, " fopen(%s, wb) failed: %s", file_name, strerror(errno));
            }
        }
        ESP_LOGW(T, "continuing with default_settings from json_settings.c ...");
        txtData = malloc(sizeof(default_settings));
        if (!txtData) {
            ESP_LOGE(T, "malloc(%d) failed: %s", sizeof(default_settings), strerror(errno));
            assert(false);
        }
        memcpy(txtData, default_settings, sizeof(default_settings));
    }

    // load txtData as .json
    if (!(root = cJSON_Parse(txtData))) {
        ESP_LOGE(T,"JSON Error in %s", file_name);
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(T, "Error before: %s", error_ptr);
        }
    }
    free(txtData);
    return root;
}

cJSON *g_settings = NULL;
cJSON *getSettings() {
    if (!g_settings) {
        g_settings = readJsonDyn(SETTINGS_FILE, true);
        if(!g_settings) {
            ESP_LOGE(T, "Error loading %s", SETTINGS_FILE);
        }
    }
    return g_settings;
}

void reloadSettings(){
    if (g_settings) {
        cJSON_Delete(g_settings);
        g_settings = NULL;
    }
    getSettings();
}

// return string from .json or default-value on error
const char *jGetS(const cJSON *json, const char *name, const char *default_val) {
    const cJSON *j = cJSON_GetObjectItemCaseSensitive(json, name);
    if(!cJSON_IsString(j)) {
        ESP_LOGE(T, "%s is not a string, falling back to %s", name, default_val);
        return default_val;
    }
    return j->valuestring;
}

// return integer from .json or default-value on error
int jGetI(cJSON *json, const char *name, int default_val) {
    const cJSON *j = cJSON_GetObjectItemCaseSensitive(json, name);
    if (!cJSON_IsNumber(j)) {
        ESP_LOGE(T, "%s is not a number, falling back to %d", name, default_val);
        return default_val;
    }
    return j->valueint;
}

// return double from .json or default-value on error
int jGetD(cJSON *json, const char *name, double default_val) {
    const cJSON *j = cJSON_GetObjectItemCaseSensitive(json, name);
    if (!cJSON_IsNumber(j)) {
        ESP_LOGE(T, "%s is not a number, falling back to %f", name, default_val);
        return default_val;
    }
    return j->valuedouble;
}

// return bool from .json or default-value on error
int jGetB(cJSON *json, const char *name, bool default_val) {
    const cJSON *j = cJSON_GetObjectItemCaseSensitive(json, name);
    if (!cJSON_IsBool(j)) {
        ESP_LOGE(T, "%s is not a bool, falling back to %s", name, default_val ? "true" : "false");
        return default_val;
    }
    return cJSON_IsTrue(j);
}
