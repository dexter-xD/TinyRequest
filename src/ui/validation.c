#include "ui_components.h"
#include "http_client.h"
#include "json_processor.h"
#include <stdio.h>
#include <string.h>

ValidationResult ValidateUrlInput(const char* url) {
    ValidationResult result = {0};

    if (!url) {
        result.is_valid = false;
        strcpy(result.error_message, "URL cannot be null");
        result.error_level = ERROR_LEVEL_ERROR;
        return result;
    }

    if (strlen(url) == 0) {
        result.is_valid = false;
        strcpy(result.error_message, "URL cannot be empty");
        result.error_level = ERROR_LEVEL_WARNING;
        return result;
    }

    if (!ValidateUrl(url)) {
        result.is_valid = false;
        strcpy(result.error_message, "Invalid URL format. Must start with http:// or https://");
        result.error_level = ERROR_LEVEL_ERROR;
        return result;
    }

    result.is_valid = true;
    return result;
}

ValidationResult ValidateJsonInput(const char* json) {
    ValidationResult result = {0};

    if (!json) {
        result.is_valid = true;
        return result;
    }

    if (strlen(json) == 0) {
        result.is_valid = true;
        return result;
    }

    if (!ValidateJson(json)) {
        result.is_valid = false;

        char* json_error = GetJsonErrorMessage();
        if (json_error) {
            snprintf(result.error_message, sizeof(result.error_message), "Invalid JSON: %s", json_error);
        } else {
            strcpy(result.error_message, "Invalid JSON format");
        }

        result.error_level = ERROR_LEVEL_WARNING;
        return result;
    }

    result.is_valid = true;
    return result;
}

ValidationResult ValidateHeaderInput(const char* header) {
    ValidationResult result = {0};

    if (!header) {
        result.is_valid = false;
        strcpy(result.error_message, "Header cannot be null");
        result.error_level = ERROR_LEVEL_ERROR;
        return result;
    }

    if (strlen(header) == 0) {
        result.is_valid = true;
        return result;
    }

    if (!strchr(header, ':')) {
        result.is_valid = false;
        strcpy(result.error_message, "Header must be in 'Key: Value' format");
        result.error_level = ERROR_LEVEL_ERROR;
        return result;
    }

    for (const char* p = header; *p; p++) {
        if (*p < 32 || *p > 126) {
            result.is_valid = false;
            strcpy(result.error_message, "Header contains invalid characters");
            result.error_level = ERROR_LEVEL_ERROR;
            return result;
        }
    }

    result.is_valid = true;
    return result;
}

void RenderValidationFeedback(Rectangle bounds, ValidationResult* validation, GruvboxTheme* theme) {
    if (!validation || !theme || validation->is_valid) return;

    Color feedback_color;
    switch (validation->error_level) {
        case ERROR_LEVEL_ERROR:
            feedback_color = theme->red;
            break;
        case ERROR_LEVEL_WARNING:
            feedback_color = theme->orange;
            break;
        case ERROR_LEVEL_INFO:
            feedback_color = theme->blue;
            break;
        case ERROR_LEVEL_SUCCESS:
            feedback_color = theme->green;
            break;
        default:
            feedback_color = theme->gray;
            break;
    }

    DrawRectangleRec(bounds, ColorAlpha(feedback_color, 0.1f));
    DrawRectangleLinesEx(bounds, 1, feedback_color);

    if (strlen(validation->error_message) > 0) {
        Vector2 text_pos = {bounds.x + PADDING_SMALL, bounds.y + PADDING_SMALL};
        DrawTextEx(GetFontDefault(), validation->error_message, text_pos, FONT_SIZE_SMALL, 1, feedback_color);
    }
}