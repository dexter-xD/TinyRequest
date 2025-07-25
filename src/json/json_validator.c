#include "json_processor.h"
#include <stdlib.h>
#include <string.h>

static char error_message[256] = {0};
static int error_position = -1;

int ValidateJson(const char* json_string) {
    if (!json_string) {
        strcpy(error_message, "JSON string is null");
        error_position = 0;
        return 0;
    }

    if (strlen(json_string) == 0) {
        error_message[0] = '\0';
        error_position = -1;
        return 1;
    }

    cJSON* json = cJSON_Parse(json_string);
    if (json == NULL) {
        const char* cjson_error = cJSON_GetErrorPtr();
        if (cjson_error != NULL) {
            error_position = (int)(cjson_error - json_string);
            snprintf(error_message, sizeof(error_message), "JSON parse error at position %d", error_position);
        } else {
            strcpy(error_message, "Invalid JSON format");
            error_position = 0;
        }
        return 0;
    }

    cJSON_Delete(json);
    error_message[0] = '\0';
    error_position = -1;
    return 1;
}

cJSON* ParseJsonSafely(const char* json_string) {
    if (!json_string || strlen(json_string) == 0) {
        return NULL;
    }

    cJSON* json = cJSON_Parse(json_string);
    if (json == NULL) {
        const char* cjson_error = cJSON_GetErrorPtr();
        if (cjson_error != NULL) {
            error_position = (int)(cjson_error - json_string);
            snprintf(error_message, sizeof(error_message), "JSON parse error at position %d", error_position);
        } else {
            strcpy(error_message, "Invalid JSON format");
            error_position = 0;
        }
    }

    return json;
}

char* GetJsonErrorMessage(void) {
    return error_message[0] != '\0' ? error_message : NULL;
}

int GetJsonErrorPosition(void) {
    return error_position;
}