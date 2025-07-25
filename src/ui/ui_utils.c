#include "ui_components.h"
#include "ui_style.h"
#include <string.h>
#include <math.h>

void DrawButton(Rectangle bounds, const char* text, bool pressed, GruvboxTheme* theme) {
    if (!text || !theme) return;
    float radius = 8.0f;

    DrawRectShadow(bounds, 6.0f, theme->bg0);

    Color bg_color = GruvboxButtonBg(theme, pressed, pressed);
    Color border_color = GruvboxButtonBorder(theme, pressed, pressed);
    DrawRoundedRect(bounds, radius, bg_color);
    DrawRoundedRectLines(bounds, radius, border_color);

    Vector2 text_size = MeasureTextEx(GetFontDefault(), text, FONT_SIZE_NORMAL, 1);
    Vector2 text_pos = {
        bounds.x + (bounds.width - text_size.x) / 2,
        bounds.y + (bounds.height - text_size.y) / 2
    };
    DrawTextEx(GetFontDefault(), text, text_pos, FONT_SIZE_NORMAL, 1, theme->fg0);
}

void DrawInputField(Rectangle bounds, const char* text, bool active, GruvboxTheme* theme) {
    if (!theme) return;
    float radius = 7.0f;

    DrawRectShadow(bounds, 4.0f, theme->bg0);

    Color bg_color = active ? theme->bg2 : GruvboxPanelBg(theme);
    Color border_color = active ? theme->blue : GruvboxPanelBorder(theme);
    DrawRoundedRect(bounds, radius, bg_color);
    DrawRoundedRectLines(bounds, radius, border_color);
    if (text && strlen(text) > 0) {
        Vector2 text_pos = {bounds.x + PADDING_MEDIUM, bounds.y + (bounds.height - FONT_SIZE_NORMAL) / 2};
        DrawTextEx(GetFontDefault(), text, text_pos, FONT_SIZE_NORMAL, 1, theme->fg0);
    }
}

void DrawDropdown(Rectangle bounds, const char** options, int option_count, int selected, bool open, GruvboxTheme* theme) {
    if (!options || !theme || option_count <= 0) return;
    float radius = 7.0f;

    DrawRectShadow(bounds, 4.0f, theme->bg0);

    Color bg_color = open ? theme->bg2 : GruvboxPanelBg(theme);
    Color border_color = open ? theme->yellow : GruvboxPanelBorder(theme);
    DrawRoundedRect(bounds, radius, bg_color);
    DrawRoundedRectLines(bounds, radius, border_color);
    if (selected >= 0 && selected < option_count) {
        Vector2 text_pos = {bounds.x + PADDING_MEDIUM, bounds.y + (bounds.height - FONT_SIZE_NORMAL) / 2};
        DrawTextEx(GetFontDefault(), options[selected], text_pos, FONT_SIZE_NORMAL, 1, theme->fg0);
    }
}

void DrawPaperPlaneIcon(Vector2 center, float size, Color color) {
    float w = size;
    float h = size * 0.7f;
    Vector2 p1 = {center.x - w * 0.5f, center.y + h * 0.5f};
    Vector2 p2 = {center.x + w * 0.5f, center.y};
    Vector2 p3 = {center.x - w * 0.5f, center.y - h * 0.5f};
    DrawTriangle(p1, p2, p3, color);
    DrawLineEx(p1, p2, 2.0f, color);
    DrawLineEx(p2, p3, 2.0f, color);
}

Rectangle GetMethodSelectorBounds(void) {
    return (Rectangle){50, 50, 120, 30};
}

Rectangle GetUrlInputBounds(void) {
    return (Rectangle){180, 50, 400, 30};
}

Rectangle GetHeadersEditorBounds(void) {
    return (Rectangle){50, 100, 530, 200};
}

Rectangle GetBodyEditorBounds(void) {
    return (Rectangle){50, 320, 530, 150};
}

Rectangle GetResponseViewerBounds(void) {
    return (Rectangle){600, 50, 400, 420};
}

Rectangle GetStatusBarBounds(void) {
    return (Rectangle){50, 480, 950, 25};
}

void RenderLoadingSpinner(Vector2 center, float radius, GruvboxTheme* theme) {
    if (!theme) return;

    static float rotation = 0.0f;
    rotation += GetFrameTime() * 360.0f;
    if (rotation > 360.0f) rotation -= 360.0f;

    const int segments = 8;
    const float segment_angle = 360.0f / segments;

    for (int i = 0; i < segments; i++) {
        float angle = rotation + (i * segment_angle);
        float alpha = 1.0f - ((float)i / segments);

        Vector2 start = {
            center.x + cosf(angle * DEG2RAD) * (radius * 0.5f),
            center.y + sinf(angle * DEG2RAD) * (radius * 0.5f)
        };
        Vector2 end = {
            center.x + cosf(angle * DEG2RAD) * radius,
            center.y + sinf(angle * DEG2RAD) * radius
        };

        Color segment_color = theme->blue;
        segment_color.a = (unsigned char)(255 * alpha);

        DrawLineEx(start, end, 3.0f, segment_color);
    }
}

void RenderProgressBar(Rectangle bounds, float progress, const char* label, GruvboxTheme* theme) {
    if (!theme) return;

    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    DrawRectangleRec(bounds, theme->bg1);
    DrawRectangleLinesEx(bounds, 1, theme->gray);

    if (progress > 0.0f) {
        Rectangle fill_bounds = {
            bounds.x + 1,
            bounds.y + 1,
            (bounds.width - 2) * progress,
            bounds.height - 2
        };
        DrawRectangleRec(fill_bounds, theme->blue);
    }

    if (label) {
        Vector2 text_size = MeasureTextEx(GetFontDefault(), label, FONT_SIZE_SMALL, 1);
        Vector2 text_pos = {
            bounds.x + (bounds.width - text_size.x) / 2,
            bounds.y + (bounds.height - text_size.y) / 2
        };
        DrawTextEx(GetFontDefault(), label, text_pos, FONT_SIZE_SMALL, 1, theme->fg0);
    }
}

void RenderRequestProgress(Rectangle bounds, AsyncHttpHandle* handle, GruvboxTheme* theme) {
    if (!handle || !theme) return;

    AsyncRequestState state = CheckAsyncRequest(handle);

    switch (state) {
        case ASYNC_REQUEST_PENDING: {

            Vector2 spinner_center = {bounds.x + 20, bounds.y + bounds.height / 2};
            RenderLoadingSpinner(spinner_center, 10, theme);

            const char* status_text = "Sending request...";
            Vector2 text_pos = {bounds.x + 45, bounds.y + (bounds.height - FONT_SIZE_SMALL) / 2};
            DrawTextEx(GetFontDefault(), status_text, text_pos, FONT_SIZE_SMALL, 1, theme->fg0);
            break;
        }
        case ASYNC_REQUEST_COMPLETED: {
            const char* status_text = "Request completed";
            Vector2 text_pos = {bounds.x + PADDING_SMALL, bounds.y + (bounds.height - FONT_SIZE_SMALL) / 2};
            DrawTextEx(GetFontDefault(), status_text, text_pos, FONT_SIZE_SMALL, 1, theme->green);
            break;
        }
        case ASYNC_REQUEST_ERROR: {
            const char* status_text = "Request failed";
            Vector2 text_pos = {bounds.x + PADDING_SMALL, bounds.y + (bounds.height - FONT_SIZE_SMALL) / 2};
            DrawTextEx(GetFontDefault(), status_text, text_pos, FONT_SIZE_SMALL, 1, theme->red);
            break;
        }
        case ASYNC_REQUEST_TIMEOUT: {
            const char* status_text = "Request timeout";
            Vector2 text_pos = {bounds.x + PADDING_SMALL, bounds.y + (bounds.height - FONT_SIZE_SMALL) / 2};
            DrawTextEx(GetFontDefault(), status_text, text_pos, FONT_SIZE_SMALL, 1, theme->orange);
            break;
        }
        case ASYNC_REQUEST_CANCELLED: {
            const char* status_text = "Request cancelled";
            Vector2 text_pos = {bounds.x + PADDING_SMALL, bounds.y + (bounds.height - FONT_SIZE_SMALL) / 2};
            DrawTextEx(GetFontDefault(), status_text, text_pos, FONT_SIZE_SMALL, 1, theme->gray);
            break;
        }
    }
}

void RenderCancelButton(Rectangle bounds, bool* cancel_requested, GruvboxTheme* theme) {
    if (!cancel_requested || !theme) return;

    bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (clicked) {
        *cancel_requested = true;
    }

    Color bg_color = hovered ? theme->red : theme->bg1;
    DrawRectangleRec(bounds, bg_color);
    DrawRectangleLinesEx(bounds, 1, theme->red);

    const char* button_text = "Cancel";
    Vector2 text_size = MeasureTextEx(GetFontDefault(), button_text, FONT_SIZE_SMALL, 1);
    Vector2 text_pos = {
        bounds.x + (bounds.width - text_size.x) / 2,
        bounds.y + (bounds.height - text_size.y) / 2
    };
    DrawTextEx(GetFontDefault(), button_text, text_pos, FONT_SIZE_SMALL, 1, theme->fg0);
}

void RenderStatusBar(Rectangle bounds, const char* status_text, GruvboxTheme* theme) {
    if (!theme) return;

    DrawRectangleRec(bounds, theme->bg2);
    DrawRectangleLinesEx(bounds, 1, theme->gray);

    if (status_text) {
        Vector2 text_pos = {bounds.x + PADDING_SMALL, bounds.y + (bounds.height - FONT_SIZE_SMALL) / 2};
        DrawTextEx(GetFontDefault(), status_text, text_pos, FONT_SIZE_SMALL, 1, theme->fg1);
    }
}

bool HandleMethodSelectorInput(Rectangle bounds, int* selected_method) {

    return false;
}

bool HandleUrlInputField(Rectangle bounds, char* url_buffer, int buffer_size) {

    return false;
}

bool HandleHeadersEditorInput(Rectangle bounds, char** headers, int* header_count) {

    return false;
}

bool HandleBodyEditorInput(Rectangle bounds, char* body_buffer, int buffer_size) {

    return false;
}