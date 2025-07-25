#include "json_processor.h"
#include <string.h>

char* FormatJson(const char* json_string) {
    if (!json_string || strlen(json_string) == 0) {
        return NULL;
    }

    cJSON* json = ParseJsonSafely(json_string);
    if (!json) {
        return NULL;
    }

    char* formatted = cJSON_Print(json);
    cJSON_Delete(json);

    return formatted;
}

char* MinifyJson(const char* json_string) {
    if (!json_string || strlen(json_string) == 0) {
        return NULL;
    }

    cJSON* json = ParseJsonSafely(json_string);
    if (!json) {
        return NULL;
    }

    char* minified = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    return minified;
}