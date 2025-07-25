#include "ui_components.h"
#include "http_client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void ShowErrorMessage(ErrorMessage* error, ErrorLevel level, const char* message) {
    if (!error || !message) return;

    error->level = level;
    strncpy(error->message, message, sizeof(error->message) - 1);
    error->message[sizeof(error->message) - 1] = '\0';
    error->display_time = 5.0f;
    error->visible = true;
}

void UpdateErrorMessage(ErrorMessage* error) {
    if (!error || !error->visible) return;

    error->display_time -= GetFrameTime();
    if (error->display_time <= 0.0f) {
        error->visible = false;
        error->message[0] = '\0';
    }
}

void RenderErrorMessage(Rectangle bounds, ErrorMessage* error, GruvboxTheme* theme) {
    if (!error || !theme || !error->visible) return;

    Color bg_color, border_color, text_color;
    switch (error->level) {
        case ERROR_LEVEL_ERROR:
            bg_color = ColorAlpha(theme->red, 0.2f);
            border_color = theme->red;
            text_color = theme->red;
            break;
        case ERROR_LEVEL_WARNING:
            bg_color = ColorAlpha(theme->orange, 0.2f);
            border_color = theme->orange;
            text_color = theme->orange;
            break;
        case ERROR_LEVEL_INFO:
            bg_color = ColorAlpha(theme->blue, 0.2f);
            border_color = theme->blue;
            text_color = theme->blue;
            break;
        case ERROR_LEVEL_SUCCESS:
            bg_color = ColorAlpha(theme->green, 0.2f);
            border_color = theme->green;
            text_color = theme->green;
            break;
        default:
            bg_color = ColorAlpha(theme->gray, 0.2f);
            border_color = theme->gray;
            text_color = theme->gray;
            break;
    }

    DrawRectangleRec(bounds, bg_color);
    DrawRectangleLinesEx(bounds, 2, border_color);

    if (strlen(error->message) > 0) {
        Vector2 text_pos = {bounds.x + PADDING_MEDIUM, bounds.y + PADDING_MEDIUM};

        const float max_width = bounds.width - (2 * PADDING_MEDIUM);
        Vector2 text_size = MeasureTextEx(GetFontDefault(), error->message, FONT_SIZE_NORMAL, 1);

        if (text_size.x <= max_width) {

            DrawTextEx(GetFontDefault(), error->message, text_pos, FONT_SIZE_NORMAL, 1, text_color);
        } else {

            char* message_copy = malloc(strlen(error->message) + 1);
            if (message_copy) {
                strcpy(message_copy, error->message);

                int break_point = (int)(strlen(message_copy) * (max_width / text_size.x));
                if (break_point < (int)strlen(message_copy)) {

                    while (break_point > 0 && message_copy[break_point] != ' ') {
                        break_point--;
                    }
                    if (break_point > 0) {
                        message_copy[break_point] = '\0';
                    }
                }

                char truncated[512];
                snprintf(truncated, sizeof(truncated), "%.400s%s", message_copy,
                        strlen(message_copy) < strlen(error->message) ? "..." : "");

                DrawTextEx(GetFontDefault(), truncated, text_pos, FONT_SIZE_NORMAL, 1, text_color);
                free(message_copy);
            }
        }
    }
}

void RenderStatusBarWithErrors(Rectangle bounds, const char* status_text, ErrorMessage* error, GruvboxTheme* theme) {
    if (!theme) return;

    if (error && error->visible) {
        RenderErrorMessage(bounds, error, theme);
        return;
    }

    RenderStatusBar(bounds, status_text, theme);
}

void HandleNetworkError(HttpError* error, ErrorMessage* ui_error) {
    if (!error || !ui_error) return;

    ErrorLevel level = ERROR_LEVEL_ERROR;
    char message[512];

    switch (error->code) {
        case HTTP_ERROR_NETWORK_FAILURE:
            level = ERROR_LEVEL_ERROR;
            snprintf(message, sizeof(message), "Network Error: %s", error->message);
            break;
        case HTTP_ERROR_TIMEOUT:
            level = ERROR_LEVEL_WARNING;
            snprintf(message, sizeof(message), "Request Timeout: %s", error->message);
            break;
        case HTTP_ERROR_SSL_ERROR:
            level = ERROR_LEVEL_ERROR;
            snprintf(message, sizeof(message), "SSL Error: %s", error->message);
            break;
        default:
            level = ERROR_LEVEL_ERROR;
            snprintf(message, sizeof(message), "HTTP Error: %s", error->message);
            break;
    }

    ShowErrorMessage(ui_error, level, message);
}

void HandleParsingError(const char* error_msg, ErrorMessage* ui_error) {
    if (!error_msg || !ui_error) return;

    char message[512];
    snprintf(message, sizeof(message), "Parsing Error: %s", error_msg);

    ShowErrorMessage(ui_error, ERROR_LEVEL_WARNING, message);
}

void HandleValidationError(const char* field_name, const char* error_msg, ErrorMessage* ui_error) {
    if (!field_name || !error_msg || !ui_error) return;

    char message[512];
    snprintf(message, sizeof(message), "%s Validation Error: %s", field_name, error_msg);

    ShowErrorMessage(ui_error, ERROR_LEVEL_WARNING, message);
}

void RenderTooltip(Vector2 position, const char* text, GruvboxTheme* theme) {
    if (!text || !theme) return;

    Vector2 text_size = MeasureTextEx(GetFontDefault(), text, FONT_SIZE_SMALL, 1);
    Rectangle tooltip_bounds = {
        position.x,
        position.y - text_size.y - PADDING_SMALL * 2,
        text_size.x + PADDING_SMALL * 2,
        text_size.y + PADDING_SMALL * 2
    };

    DrawRectangleRec(tooltip_bounds, theme->bg2);
    DrawRectangleLinesEx(tooltip_bounds, 1, theme->gray);

    Vector2 text_pos = {
        tooltip_bounds.x + PADDING_SMALL,
        tooltip_bounds.y + PADDING_SMALL
    };
    DrawTextEx(GetFontDefault(), text, text_pos, FONT_SIZE_SMALL, 1, theme->fg0);
}

void RenderErrorIcon(Vector2 position, float size, GruvboxTheme* theme) {
    if (!theme) return;

    Vector2 center = {position.x + size/2, position.y + size/2};
    float radius = size / 2 - 2;

    DrawCircleV(center, radius, theme->red);
    DrawCircleLinesV(center, radius, theme->bg0);

    float cross_size = radius * 0.6f;
    DrawLineEx(
        (Vector2){center.x - cross_size, center.y - cross_size},
        (Vector2){center.x + cross_size, center.y + cross_size},
        2, theme->bg0
    );
    DrawLineEx(
        (Vector2){center.x + cross_size, center.y - cross_size},
        (Vector2){center.x - cross_size, center.y + cross_size},
        2, theme->bg0
    );
}

void RenderWarningIcon(Vector2 position, float size, GruvboxTheme* theme) {
    if (!theme) return;

    Vector2 center = {position.x + size/2, position.y + size/2};
    float half_size = size / 2 - 2;

    Vector2 top = {center.x, center.y - half_size};
    Vector2 bottom_left = {center.x - half_size, center.y + half_size};
    Vector2 bottom_right = {center.x + half_size, center.y + half_size};

    DrawTriangle(top, bottom_left, bottom_right, theme->orange);
    DrawTriangleLines(top, bottom_left, bottom_right, theme->bg0);

    DrawRectangle((int)(center.x - 1), (int)(center.y - half_size * 0.3f), 2, (int)(half_size * 0.4f), theme->bg0);
    DrawRectangle((int)(center.x - 1), (int)(center.y + half_size * 0.2f), 2, 2, theme->bg0);
}

void RenderSuccessIcon(Vector2 position, float size, GruvboxTheme* theme) {
    if (!theme) return;

    Vector2 center = {position.x + size/2, position.y + size/2};
    float radius = size / 2 - 2;

    DrawCircleV(center, radius, theme->green);
    DrawCircleLinesV(center, radius, theme->bg0);

    float check_size = radius * 0.6f;
    Vector2 check_start = {center.x - check_size * 0.5f, center.y};
    Vector2 check_mid = {center.x - check_size * 0.1f, center.y + check_size * 0.3f};
    Vector2 check_end = {center.x + check_size * 0.5f, center.y - check_size * 0.3f};

    DrawLineEx(check_start, check_mid, 2, theme->bg0);
    DrawLineEx(check_mid, check_end, 2, theme->bg0);
}

void RenderInfoIcon(Vector2 position, float size, GruvboxTheme* theme) {
    if (!theme) return;

    Vector2 center = {position.x + size/2, position.y + size/2};
    float radius = size / 2 - 2;

    DrawCircleV(center, radius, theme->blue);
    DrawCircleLinesV(center, radius, theme->bg0);

    DrawRectangle((int)(center.x - 1), (int)(center.y - radius * 0.5f), 2, 2, theme->bg0);
    DrawRectangle((int)(center.x - 1), (int)(center.y - radius * 0.2f), 2, (int)(radius * 0.6f), theme->bg0);
}