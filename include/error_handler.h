#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include "ui_components.h"
#include "http_client.h"
#include "request_state.h"
#include "theme.h"

typedef struct {
    ErrorMessage network_error;
    ErrorMessage validation_error;
    ErrorMessage parsing_error;
    ErrorMessage general_error;
    bool has_active_errors;
} AppErrorHandler;

AppErrorHandler* CreateErrorHandler(void);
void FreeErrorHandler(AppErrorHandler* handler);
void ClearAllErrors(AppErrorHandler* handler);

void ReportNetworkError(AppErrorHandler* handler, HttpError* http_error);
void ReportValidationError(AppErrorHandler* handler, const char* field_name, ValidationResult* validation);
void ReportParsingError(AppErrorHandler* handler, const char* context, const char* error_msg);
void ReportGeneralError(AppErrorHandler* handler, ErrorLevel level, const char* message);

void UpdateErrorHandler(AppErrorHandler* handler);
bool HasActiveErrors(AppErrorHandler* handler);
ErrorMessage* GetMostRecentError(AppErrorHandler* handler);

void RenderErrorHandler(AppErrorHandler* handler, Rectangle status_bounds, GruvboxTheme* theme);
void RenderErrorTooltips(AppErrorHandler* handler, GruvboxTheme* theme);

void ValidateAndReportUrl(AppErrorHandler* handler, const char* url);
void ValidateAndReportJson(AppErrorHandler* handler, const char* json);
void ValidateAndReportHeaders(AppErrorHandler* handler, char** headers, int header_count);

typedef struct {
    char suggestion[256];
    bool can_auto_fix;
    void (*auto_fix_callback)(void* context);
    void* fix_context;
} ErrorRecoverySuggestion;

ErrorRecoverySuggestion GetRecoverySuggestion(HttpError* error);
void ApplyAutoFix(ErrorRecoverySuggestion* suggestion);

#endif