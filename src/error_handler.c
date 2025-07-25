#include "error_handler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

AppErrorHandler* CreateErrorHandler(void) {
    AppErrorHandler* handler = malloc(sizeof(AppErrorHandler));
    if (!handler) return NULL;

    memset(&handler->network_error, 0, sizeof(ErrorMessage));
    memset(&handler->validation_error, 0, sizeof(ErrorMessage));
    memset(&handler->parsing_error, 0, sizeof(ErrorMessage));
    memset(&handler->general_error, 0, sizeof(ErrorMessage));

    handler->has_active_errors = false;

    return handler;
}

void FreeErrorHandler(AppErrorHandler* handler) {
    if (!handler) return;
    free(handler);
}

void ClearAllErrors(AppErrorHandler* handler) {
    if (!handler) return;

    handler->network_error.visible = false;
    handler->validation_error.visible = false;
    handler->parsing_error.visible = false;
    handler->general_error.visible = false;
    handler->has_active_errors = false;
}

void ReportNetworkError(AppErrorHandler* handler, HttpError* http_error) {
    if (!handler || !http_error) return;

    printf("DEBUG: ReportNetworkError called with error code: %d, message: %s\n",
           http_error->code, http_error->message);
    HandleNetworkError(http_error, &handler->network_error);
    handler->has_active_errors = true;
}

void ReportValidationError(AppErrorHandler* handler, const char* field_name, ValidationResult* validation) {
    if (!handler || !validation || validation->is_valid) return;

    HandleValidationError(field_name, validation->error_message, &handler->validation_error);
    handler->has_active_errors = true;
}

void ReportParsingError(AppErrorHandler* handler, const char* context, const char* error_msg) {
    if (!handler) return;

    char full_message[512];
    if (context && error_msg) {
        snprintf(full_message, sizeof(full_message), "%s: %s", context, error_msg);
    } else if (error_msg) {
        strncpy(full_message, error_msg, sizeof(full_message) - 1);
        full_message[sizeof(full_message) - 1] = '\0';
    } else {
        strcpy(full_message, "Parsing error occurred");
    }

    HandleParsingError(full_message, &handler->parsing_error);
    handler->has_active_errors = true;
}

void ReportGeneralError(AppErrorHandler* handler, ErrorLevel level, const char* message) {
    if (!handler || !message) return;

    ShowErrorMessage(&handler->general_error, level, message);
    handler->has_active_errors = true;
}

void UpdateErrorHandler(AppErrorHandler* handler) {
    if (!handler) return;

    UpdateErrorMessage(&handler->network_error);
    UpdateErrorMessage(&handler->validation_error);
    UpdateErrorMessage(&handler->parsing_error);
    UpdateErrorMessage(&handler->general_error);

    handler->has_active_errors = (
        handler->network_error.visible ||
        handler->validation_error.visible ||
        handler->parsing_error.visible ||
        handler->general_error.visible
    );
}

bool HasActiveErrors(AppErrorHandler* handler) {
    if (!handler) return false;
    return handler->has_active_errors;
}

ErrorMessage* GetMostRecentError(AppErrorHandler* handler) {
    if (!handler) return NULL;

    ErrorMessage* most_recent = NULL;
    float highest_time = 0.0f;

    if (handler->network_error.visible && handler->network_error.display_time > highest_time) {
        most_recent = &handler->network_error;
        highest_time = handler->network_error.display_time;
    }

    if (handler->validation_error.visible && handler->validation_error.display_time > highest_time) {
        most_recent = &handler->validation_error;
        highest_time = handler->validation_error.display_time;
    }

    if (handler->parsing_error.visible && handler->parsing_error.display_time > highest_time) {
        most_recent = &handler->parsing_error;
        highest_time = handler->parsing_error.display_time;
    }

    if (handler->general_error.visible && handler->general_error.display_time > highest_time) {
        most_recent = &handler->general_error;
        highest_time = handler->general_error.display_time;
    }

    return most_recent;
}

void RenderErrorHandler(AppErrorHandler* handler, Rectangle status_bounds, GruvboxTheme* theme) {
    if (!handler || !theme) return;

    ErrorMessage* current_error = GetMostRecentError(handler);
    if (current_error) {
        RenderErrorMessage(status_bounds, current_error, theme);
    } else {

        RenderStatusBar(status_bounds, "Ready", theme);
    }
}

void RenderErrorTooltips(AppErrorHandler* handler, GruvboxTheme* theme) {
    if (!handler || !theme) return;

    Vector2 mouse_pos = GetMousePosition();

    if (handler->validation_error.visible) {
        RenderTooltip(mouse_pos, handler->validation_error.message, theme);
    }
}

void ValidateAndReportUrl(AppErrorHandler* handler, const char* url) {
    if (!handler) return;

    ValidationResult validation = ValidateUrlInput(url);
    if (!validation.is_valid) {
        ReportValidationError(handler, "URL", &validation);
    }
}

void ValidateAndReportJson(AppErrorHandler* handler, const char* json) {
    if (!handler) return;

    ValidationResult validation = ValidateJsonInput(json);
    if (!validation.is_valid) {
        ReportValidationError(handler, "JSON Body", &validation);
    }
}

void ValidateAndReportHeaders(AppErrorHandler* handler, char** headers, int header_count) {
    if (!handler || !headers) return;

    for (int i = 0; i < header_count; i++) {
        if (headers[i] && strlen(headers[i]) > 0) {
            ValidationResult validation = ValidateHeaderInput(headers[i]);
            if (!validation.is_valid) {
                char field_name[64];
                snprintf(field_name, sizeof(field_name), "Header %d", i + 1);
                ReportValidationError(handler, field_name, &validation);
                break;
            }
        }
    }
}

ErrorRecoverySuggestion GetRecoverySuggestion(HttpError* error) {
    ErrorRecoverySuggestion suggestion = {"", false, NULL, NULL};

    if (!error) return suggestion;

    switch (error->code) {
        case HTTP_ERROR_INVALID_URL:
            strcpy(suggestion.suggestion, "Check URL format - ensure it starts with http:// or https://");
            suggestion.can_auto_fix = false;
            break;

        case HTTP_ERROR_NETWORK_FAILURE:
            strcpy(suggestion.suggestion, "Check internet connection and try again");
            suggestion.can_auto_fix = false;
            break;

        case HTTP_ERROR_TIMEOUT:
            strcpy(suggestion.suggestion, "Server is slow - try increasing timeout or check server status");
            suggestion.can_auto_fix = false;
            break;

        case HTTP_ERROR_SSL_ERROR:
            strcpy(suggestion.suggestion, "SSL certificate issue - try using HTTP instead of HTTPS for testing");
            suggestion.can_auto_fix = false;
            break;

        case HTTP_ERROR_OUT_OF_MEMORY:
            strcpy(suggestion.suggestion, "Close other applications to free memory");
            suggestion.can_auto_fix = false;
            break;

        case HTTP_ERROR_CANCELLED:
            strcpy(suggestion.suggestion, "Request was cancelled - click Send to try again");
            suggestion.can_auto_fix = false;
            break;

        default:
            strcpy(suggestion.suggestion, "Try the request again or check server logs");
            suggestion.can_auto_fix = false;
            break;
    }

    return suggestion;
}

void ApplyAutoFix(ErrorRecoverySuggestion* suggestion) {
    if (!suggestion || !suggestion->can_auto_fix || !suggestion->auto_fix_callback) {
        return;
    }

    suggestion->auto_fix_callback(suggestion->fix_context);
}