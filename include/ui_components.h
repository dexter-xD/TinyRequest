#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include "raylib.h"
#include "theme.h"
#include "http_client.h"

void RenderMethodSelector(Rectangle bounds, int* selected_method, GruvboxTheme* theme);
void RenderUrlInput(Rectangle bounds, char* url_buffer, int buffer_size, GruvboxTheme* theme);
void RenderHeadersEditor(Rectangle bounds, char** headers, int* header_count, GruvboxTheme* theme);
void RenderBodyEditor(Rectangle bounds, char* body_buffer, int buffer_size, GruvboxTheme* theme);
void RenderResponseViewer(Rectangle bounds, HttpResponse* response, GruvboxTheme* theme);
void RenderStatusBar(Rectangle bounds, const char* status_text, GruvboxTheme* theme);
bool IsMethodDropdownOpen(void);
void RenderMethodDropdownOverlay(Rectangle bounds, int* selected_method, GruvboxTheme* theme);

bool HandleMethodSelectorInput(Rectangle bounds, int* selected_method);
bool HandleUrlInputField(Rectangle bounds, char* url_buffer, int buffer_size);
bool HandleHeadersEditorInput(Rectangle bounds, char** headers, int* header_count);
bool HandleBodyEditorInput(Rectangle bounds, char* body_buffer, int buffer_size);

void DrawButton(Rectangle bounds, const char* text, bool pressed, GruvboxTheme* theme);
void DrawInputField(Rectangle bounds, const char* text, bool active, GruvboxTheme* theme);
void DrawDropdown(Rectangle bounds, const char** options, int option_count, int selected, bool open, GruvboxTheme* theme);

Rectangle GetMethodSelectorBounds(void);
Rectangle GetUrlInputBounds(void);
Rectangle GetHeadersEditorBounds(void);
Rectangle GetBodyEditorBounds(void);
Rectangle GetResponseViewerBounds(void);
Rectangle GetStatusBarBounds(void);

void RenderLoadingSpinner(Vector2 center, float radius, GruvboxTheme* theme);
void RenderProgressBar(Rectangle bounds, float progress, const char* label, GruvboxTheme* theme);
void RenderRequestProgress(Rectangle bounds, AsyncHttpHandle* handle, GruvboxTheme* theme);
void RenderCancelButton(Rectangle bounds, bool* cancel_requested, GruvboxTheme* theme);

typedef enum {
    ERROR_LEVEL_INFO = 0,
    ERROR_LEVEL_WARNING,
    ERROR_LEVEL_ERROR,
    ERROR_LEVEL_SUCCESS
} ErrorLevel;

typedef struct {
    ErrorLevel level;
    char message[512];
    float display_time;
    bool visible;
} ErrorMessage;

void ShowErrorMessage(ErrorMessage* error, ErrorLevel level, const char* message);
void UpdateErrorMessage(ErrorMessage* error);
void RenderErrorMessage(Rectangle bounds, ErrorMessage* error, GruvboxTheme* theme);
void RenderStatusBarWithErrors(Rectangle bounds, const char* status_text, ErrorMessage* error, GruvboxTheme* theme);

typedef struct {
    bool is_valid;
    char error_message[256];
    ErrorLevel error_level;
} ValidationResult;

ValidationResult ValidateUrlInput(const char* url);
ValidationResult ValidateJsonInput(const char* json);
ValidationResult ValidateHeaderInput(const char* header);
void RenderValidationFeedback(Rectangle bounds, ValidationResult* validation, GruvboxTheme* theme);

void HandleNetworkError(HttpError* error, ErrorMessage* ui_error);
void HandleParsingError(const char* error_msg, ErrorMessage* ui_error);
void HandleValidationError(const char* field_name, const char* error_msg, ErrorMessage* ui_error);

void RenderTooltip(Vector2 position, const char* text, GruvboxTheme* theme);
void RenderErrorIcon(Vector2 position, float size, GruvboxTheme* theme);
void RenderWarningIcon(Vector2 position, float size, GruvboxTheme* theme);
void RenderSuccessIcon(Vector2 position, float size, GruvboxTheme* theme);
void RenderInfoIcon(Vector2 position, float size, GruvboxTheme* theme);

void CleanupUIComponents(void);

struct MemoryPool;

#endif