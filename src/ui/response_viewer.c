#include "ui_components.h"
#include "memory_manager.h"
#include "json_processor.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    bool is_valid;
    Rectangle bounds;
    char* cached_text;
    size_t text_hash;
    double last_update;
} UICache;

static UICache response_cache = {0};
static MemoryPool* ui_memory_pool = NULL;

static float header_scroll_y = 0.0f;
static float header_scroll_x = 0.0f;

static bool dragging_vbar = false;
static float drag_vbar_start = 0.0f;
static float drag_vbar_offset = 0.0f;
static bool dragging_hbar = false;
static float drag_hbar_start = 0.0f;
static float drag_hbar_offset = 0.0f;

static void RenderResponseBodyOptimized(Rectangle bounds, HttpResponse* response, GruvboxTheme* theme, bool use_cache);
static void RenderLargeResponseVirtual(Rectangle bounds, const char* content, GruvboxTheme* theme);
static void RenderPlainTextOptimized(Rectangle bounds, const char* text, GruvboxTheme* theme);
static void RenderResponseStatus(Rectangle bounds, HttpResponse* response, GruvboxTheme* theme);
static void RenderResponseHeaders(Rectangle bounds, HttpResponse* response, GruvboxTheme* theme);
static void RenderScrollbar(Rectangle bounds, float scroll_offset, float max_scroll, GruvboxTheme* theme);
static size_t SimpleStringHash(const char* str);
static bool IsJsonResponse(HttpResponse* response);
static Color GetStatusCodeColor(int status_code, GruvboxTheme* theme);

static const char* GetStatusPhrase(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default: return "";
    }
}

void RenderResponseViewer(Rectangle bounds, HttpResponse* response, GruvboxTheme* theme) {
    if (!theme) return;

    DrawRectangleRec(bounds, theme->bg1);
    DrawRectangleLinesEx(bounds, 1, theme->gray);

    if (!response) {

        const char* placeholder = "Response will appear here after sending a request";
        Vector2 text_size = MeasureTextEx(GetUIFont(), placeholder, FONT_SIZE_NORMAL, 1);
        Vector2 text_pos = {
            bounds.x + (bounds.width - text_size.x) / 2,
            bounds.y + (bounds.height - text_size.y) / 2
        };
        DrawTextEx(GetUIFont(), placeholder, text_pos, FONT_SIZE_NORMAL, 1, theme->gray);
        return;
    }

    if (!ui_memory_pool) {
        ui_memory_pool = CreateMemoryPool(256 * 1024);
    }

    size_t content_hash = 0;
    if (response->body) {
        content_hash = SimpleStringHash(response->body);
    }

    bool use_cache = false;

    response_cache.is_valid = false;
    if (response_cache.cached_text) {
        free(response_cache.cached_text);
        response_cache.cached_text = NULL;
    }

    Rectangle status_bounds = {bounds.x, bounds.y, bounds.width, 38};
    RenderResponseStatus(status_bounds, response, theme);

    static int active_tab = 0;
    const char* tab_names[] = {"Preview", "Headers"};
    int tab_count = 2;
    float tab_h = 28;
    float tab_w = 100;
    float tab_spacing = 8;
    float outer_margin = 10.0f;
    float tab_x = bounds.x + outer_margin;
    float tab_y = bounds.y + status_bounds.height + outer_margin;
    for (int i = 0; i < tab_count; ++i) {
        Rectangle tab_rect = {tab_x + i * (tab_w + tab_spacing), tab_y, tab_w, tab_h};
        bool hovered = CheckCollisionPointRec(GetMousePosition(), tab_rect);
        bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        Color tab_bg = (active_tab == i) ? theme->bg2 : theme->bg1;
        Color tab_border = (active_tab == i) ? theme->orange : theme->gray;
        DrawRectangleRec(tab_rect, tab_bg);
        DrawRectangleLinesEx(tab_rect, 2, tab_border);
        Vector2 label_size = MeasureTextEx(GetUIFont(), tab_names[i], FONT_SIZE_NORMAL, 1);
        Vector2 label_pos = {tab_rect.x + (tab_rect.width - label_size.x) / 2, tab_rect.y + (tab_rect.height - label_size.y) / 2};
        DrawTextEx(GetUIFont(), tab_names[i], label_pos, FONT_SIZE_NORMAL, 1, (active_tab == i) ? theme->orange : theme->fg1);
        if (clicked) active_tab = i;
    }

    float divider_y = tab_y + tab_h + outer_margin;
    DrawLineEx((Vector2){bounds.x, divider_y}, (Vector2){bounds.x + bounds.width, divider_y}, 1.5f, theme->gray);
    float content_y = divider_y + outer_margin;
    float content_h = bounds.height - (content_y - bounds.y);
    Rectangle content_bounds = {bounds.x, content_y, bounds.width, content_h};
    if (active_tab == 0) {

        RenderResponseBodyOptimized(content_bounds, response, theme, use_cache);
    } else {

        RenderResponseHeaders(content_bounds, response, theme);
    }
}

static void RenderResponseBodyOptimized(Rectangle bounds, HttpResponse* response, GruvboxTheme* theme, bool use_cache) {
    if (!response || !response->body) return;

    size_t body_length = strlen(response->body);

    if (body_length > 1024 * 1024) {
        RenderLargeResponseVirtual(bounds, response->body, theme);
        return;
    }

    bool is_json = IsJsonResponse(response);

    if (is_json) {

        RenderJsonWithHighlighting(bounds, response->body, theme);
    } else {

        RenderPlainTextOptimized(bounds, response->body, theme);
    }
}

static void RenderLargeResponseVirtual(Rectangle bounds, const char* content, GruvboxTheme* theme) {
    static float scroll_offset = 0.0f;
    static int visible_lines = 0;

    const float line_height = FONT_SIZE_SMALL + 2;
    visible_lines = (int)(bounds.height / line_height) + 2;

    float wheel = GetMouseWheelMove();
    if (CheckCollisionPointRec(GetMousePosition(), bounds)) {
        scroll_offset -= wheel * line_height * 3;
        if (scroll_offset < 0) scroll_offset = 0;
    }

    int start_line = (int)(scroll_offset / line_height);
    int end_line = start_line + visible_lines;

    int total_lines = 1;
    for (const char* p = content; *p; p++) {
        if (*p == '\n') total_lines++;
    }

    float max_scroll = (total_lines - visible_lines + 1) * line_height;
    if (max_scroll < 0) max_scroll = 0;
    if (scroll_offset > max_scroll) scroll_offset = max_scroll;

    const char* line_start = content;
    int current_line = 0;

    while (current_line < start_line && *line_start) {
        if (*line_start == '\n') {
            current_line++;
        }
        line_start++;
    }

    Vector2 text_pos = {bounds.x + PADDING_SMALL, bounds.y};
    const char* current = line_start;

    BeginScissorMode((int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height);

    for (int line = start_line; line < end_line && *current; line++) {
        const char* line_end = strchr(current, '\n');
        if (!line_end) line_end = current + strlen(current);

        size_t line_len = line_end - current;
        if (line_len > 0) {

            StringBuffer* line_buffer = CreateStringBuffer(line_len + 1);
            if (line_buffer) {
                char* temp_line = malloc(line_len + 1);
                if (temp_line) {
                    strncpy(temp_line, current, line_len);
                    temp_line[line_len] = '\0';

                    StringBufferAppend(line_buffer, temp_line);
                    DrawTextEx(GetCodeFont(), StringBufferData(line_buffer), text_pos, FONT_SIZE_SMALL, 1, theme->fg0);

                    free(temp_line);
                }
                FreeStringBuffer(line_buffer);
            }
        }

        text_pos.y += line_height;
        current = line_end;
        if (*current == '\n') current++;
    }

    EndScissorMode();

    if (total_lines > visible_lines) {
        RenderScrollbar(bounds, scroll_offset, max_scroll, theme);
    }
}

static void RenderPlainTextOptimized(Rectangle bounds, const char* text, GruvboxTheme* theme) {
    if (!text) return;

    const size_t CHUNK_SIZE = 1024;
    size_t text_len = strlen(text);
    Vector2 current_pos = {bounds.x + PADDING_SMALL, bounds.y + PADDING_SMALL};

    BeginScissorMode((int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height);

    for (size_t i = 0; i < text_len; i += CHUNK_SIZE) {
        size_t chunk_len = CHUNK_SIZE;
        if (i + chunk_len > text_len) {
            chunk_len = text_len - i;
        }

        char* chunk = malloc(chunk_len + 1);
        if (chunk) {
            strncpy(chunk, text + i, chunk_len);
            chunk[chunk_len] = '\0';

            DrawTextEx(GetCodeFont(), chunk, current_pos, FONT_SIZE_SMALL, 1, theme->fg0);

            current_pos.y += FONT_SIZE_SMALL + 2;

            free(chunk);
        }

        if (current_pos.y > bounds.y + bounds.height + 50) {
            break;
        }
    }

    EndScissorMode();
}

static void RenderResponseStatus(Rectangle bounds, HttpResponse* response, GruvboxTheme* theme) {
    Font badge_font = GetCodeFont();
    int badge_font_size = FONT_SIZE_NORMAL + 1;
    int badge_font_size_small = FONT_SIZE_SMALL + 1;
    float outer_margin = 10.0f;

    Color status_color = GetStatusCodeColor(response->status_code, theme);
    char status_line[64];
    snprintf(status_line, sizeof(status_line), "%d %s", response->status_code, GetStatusPhrase(response->status_code));
    Vector2 line_size = MeasureTextEx(badge_font, status_line, badge_font_size, 1);
    float badge_h = line_size.y + 6;
    float badge_w = line_size.x + 18;
    float badge_y = bounds.y + (bounds.height - badge_h) / 2 + 2;
    Rectangle status_badge = {bounds.x + outer_margin, badge_y, badge_w, badge_h};
    DrawRectangleRec(status_badge, status_color);
    DrawTextEx(badge_font, status_line,
        (Vector2){status_badge.x + (badge_w - line_size.x) / 2, status_badge.y + (badge_h - line_size.y) / 2},
        badge_font_size, 1, theme->bg0);

    char time_text[32];
    snprintf(time_text, sizeof(time_text), "%.2f ms", response->response_time);
    Vector2 time_size = MeasureTextEx(badge_font, time_text, badge_font_size_small, 1);
    float time_badge_w = time_size.x + 14;
    float time_badge_h = time_size.y + 4;
    float time_badge_y = bounds.y + (bounds.height - time_badge_h) / 2 + 2;
    Rectangle time_badge = {bounds.x + bounds.width - time_badge_w - outer_margin, time_badge_y, time_badge_w, time_badge_h};
    DrawRectangleRec(time_badge, BlendColors(theme->bg2, theme->yellow, 0.10f));
    DrawTextEx(badge_font, time_text,
        (Vector2){time_badge.x + (time_badge_w - time_size.x) / 2, time_badge.y + (time_badge_h - time_size.y) / 2},
        badge_font_size_small, 1, theme->yellow);

    if (response->body) {
        size_t body_len = strlen(response->body);
        char size_text[32];
        if (body_len > 1024 * 1024) {
            snprintf(size_text, sizeof(size_text), "%.2f MB", body_len / (1024.0 * 1024.0));
        } else if (body_len > 1024) {
            snprintf(size_text, sizeof(size_text), "%.2f KB", body_len / 1024.0);
        } else {
            snprintf(size_text, sizeof(size_text), "%zu B", body_len);
        }
        Vector2 size_size = MeasureTextEx(badge_font, size_text, badge_font_size_small, 1);
        float size_badge_w = size_size.x + 14;
        float size_badge_h = size_size.y + 4;
        float size_badge_y = bounds.y + (bounds.height - size_badge_h) / 2 + 2;
        Rectangle size_badge = {time_badge.x - size_badge_w - 8, size_badge_y, size_badge_w, size_badge_h};
        DrawRectangleRec(size_badge, BlendColors(theme->bg2, theme->blue, 0.10f));
        DrawTextEx(badge_font, size_text,
            (Vector2){size_badge.x + (size_badge_w - size_size.x) / 2, size_badge.y + (size_badge_h - size_size.y) / 2},
            badge_font_size_small, 1, theme->blue);
    }
}

static void RenderResponseHeaders(Rectangle bounds, HttpResponse* response, GruvboxTheme* theme) {
    float outer_margin = 10.0f;
    float content_padding = outer_margin;

    char header_title[64];
    snprintf(header_title, sizeof(header_title), "Headers (%d)", response->header_count);
    Vector2 title_pos = {bounds.x + content_padding, bounds.y};
    DrawTextEx(GetUIFont(), header_title, title_pos, FONT_SIZE_SMALL, 1, theme->fg0);

    float y = bounds.y + FONT_SIZE_SMALL + 10;
    float max_line_width = 0.0f;
    float total_height = 0.0f;
    for (int i = 0; i < response->header_count; i++) {
        if (response->headers[i]) {
            Vector2 sz = MeasureTextEx(GetCodeFont(), response->headers[i], FONT_SIZE_SMALL, 1);
            if (sz.x > max_line_width) max_line_width = sz.x;
            total_height += FONT_SIZE_SMALL + 3;
        }
    }

    float view_h = bounds.height - (FONT_SIZE_SMALL + 10);
    float view_w = bounds.width - 2 * content_padding;
    float max_scroll_y = (total_height > view_h) ? (total_height - view_h) : 0.0f;
    float max_scroll_x = (max_line_width > view_w) ? (max_line_width - view_w) : 0.0f;

    if (CheckCollisionPointRec(GetMousePosition(), bounds)) {
        float wheel = GetMouseWheelMove();
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
            header_scroll_x -= wheel * 32.0f;
        } else {
            header_scroll_y -= wheel * 32.0f;
        }
    }
    if (header_scroll_y < 0) header_scroll_y = 0;
    if (header_scroll_y > max_scroll_y) header_scroll_y = max_scroll_y;
    if (header_scroll_x < 0) header_scroll_x = 0;
    if (header_scroll_x > max_scroll_x) header_scroll_x = max_scroll_x;

    int scissor_x = (int)bounds.x;
    int scissor_y = (int)bounds.y;
    int scissor_w = (int)bounds.width;
    int scissor_h = (int)bounds.height;
    BeginScissorMode(scissor_x, scissor_y, scissor_w, scissor_h);

    float draw_y = y - header_scroll_y;
    for (int i = 0; i < response->header_count; i++) {
        if (response->headers[i]) {
            DrawTextEx(
                GetCodeFont(),
                response->headers[i],
                (Vector2){bounds.x + content_padding - header_scroll_x, draw_y},
                FONT_SIZE_SMALL, 1, theme->fg1);
            draw_y += FONT_SIZE_SMALL + 3;
            if (draw_y > bounds.y + bounds.height - FONT_SIZE_SMALL) break;
        }
    }
    EndScissorMode();

    if (max_scroll_y > 0.0f) {
        Rectangle vbar = {bounds.x + bounds.width - 12, bounds.y + FONT_SIZE_SMALL + 10, 12, view_h};
        float thumb_ratio = view_h / (max_scroll_y + view_h);
        if (thumb_ratio > 1.0f) thumb_ratio = 1.0f;
        float thumb_h = vbar.height * thumb_ratio;
        float thumb_y = vbar.y + (header_scroll_y / max_scroll_y) * (vbar.height - thumb_h);
        Rectangle thumb = {vbar.x + 2, thumb_y, vbar.width - 4, thumb_h};
        bool hovered = CheckCollisionPointRec(GetMousePosition(), thumb);
        bool pressed = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (pressed) {
            dragging_vbar = true;
            drag_vbar_start = GetMousePosition().y;
            drag_vbar_offset = header_scroll_y;
        }
        if (dragging_vbar && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float delta = GetMousePosition().y - drag_vbar_start;
            float scroll_range = vbar.height - thumb_h;
            if (scroll_range > 0) {
                header_scroll_y = drag_vbar_offset + (delta / scroll_range) * max_scroll_y;
                if (header_scroll_y < 0) header_scroll_y = 0;
                if (header_scroll_y > max_scroll_y) header_scroll_y = max_scroll_y;
            }
        }
        if (dragging_vbar && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            dragging_vbar = false;
        }
        Color thumb_color = hovered || dragging_vbar ? theme->fg1 : theme->gray;
        DrawRectangleRec(vbar, theme->bg1);
        DrawRectangleLinesEx(vbar, 1, theme->gray);
        DrawRectangleRec(thumb, thumb_color);
    }

    if (max_scroll_x > 0.0f) {
        Rectangle hbar = {bounds.x + content_padding, bounds.y + bounds.height - 12, view_w, 12};
        float thumb_ratio = view_w / (max_scroll_x + view_w);
        if (thumb_ratio > 1.0f) thumb_ratio = 1.0f;
        float thumb_w = hbar.width * thumb_ratio;
        float thumb_x = hbar.x + (header_scroll_x / max_scroll_x) * (hbar.width - thumb_w);
        Rectangle thumb = {thumb_x, hbar.y + 2, thumb_w, hbar.height - 4};
        bool hovered = CheckCollisionPointRec(GetMousePosition(), thumb);
        bool pressed = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (pressed) {
            dragging_hbar = true;
            drag_hbar_start = GetMousePosition().x;
            drag_hbar_offset = header_scroll_x;
        }
        if (dragging_hbar && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float delta = GetMousePosition().x - drag_hbar_start;
            float scroll_range = hbar.width - thumb_w;
            if (scroll_range > 0) {
                header_scroll_x = drag_hbar_offset + (delta / scroll_range) * max_scroll_x;
                if (header_scroll_x < 0) header_scroll_x = 0;
                if (header_scroll_x > max_scroll_x) header_scroll_x = max_scroll_x;
            }
        }
        if (dragging_hbar && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            dragging_hbar = false;
        }
        Color thumb_color = hovered || dragging_hbar ? theme->fg1 : theme->gray;
        DrawRectangleRec(hbar, theme->bg1);
        DrawRectangleLinesEx(hbar, 1, theme->gray);
        DrawRectangleRec(thumb, thumb_color);
    }
}

static void RenderScrollbar(Rectangle bounds, float scroll_offset, float max_scroll, GruvboxTheme* theme) {
    const float scrollbar_width = 12;
    Rectangle scrollbar_track = {
        bounds.x + bounds.width - scrollbar_width,
        bounds.y,
        scrollbar_width,
        bounds.height
    };

    DrawRectangleRec(scrollbar_track, theme->bg1);
    DrawRectangleLinesEx(scrollbar_track, 1, theme->gray);

    float thumb_ratio = bounds.height / (max_scroll + bounds.height);
    if (thumb_ratio > 1.0f) thumb_ratio = 1.0f;

    float thumb_height = scrollbar_track.height * thumb_ratio;
    float thumb_y = scrollbar_track.y + (scroll_offset / max_scroll) * (scrollbar_track.height - thumb_height);

    Rectangle scrollbar_thumb = {
        scrollbar_track.x + 2,
        thumb_y,
        scrollbar_width - 4,
        thumb_height
    };

    bool thumb_hovered = CheckCollisionPointRec(GetMousePosition(), scrollbar_thumb);
    Color thumb_color = thumb_hovered ? theme->fg1 : theme->gray;
    DrawRectangleRec(scrollbar_thumb, thumb_color);
}

static size_t SimpleStringHash(const char* str) {
    size_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static bool IsJsonResponse(HttpResponse* response) {
    if (!response || !response->headers) return false;

    for (int i = 0; i < response->header_count; i++) {
        if (response->headers[i] && strstr(response->headers[i], "application/json")) {
            return true;
        }
    }

    if (response->body) {
        const char* trimmed = response->body;
        while (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n') trimmed++;
        return (*trimmed == '{' || *trimmed == '[');
    }

    return false;
}

static Color GetStatusCodeColor(int status_code, GruvboxTheme* theme) {
    if (status_code >= 200 && status_code < 300) {
        return theme->green;
    } else if (status_code >= 300 && status_code < 400) {
        return theme->yellow;
    } else if (status_code >= 400 && status_code < 500) {
        return theme->orange;
    } else if (status_code >= 500) {
        return theme->red;
    } else {
        return theme->gray;
    }
}

void CleanupUIComponents(void) {
    if (ui_memory_pool) {
        FreeMemoryPool(ui_memory_pool);
        ui_memory_pool = NULL;
    }

    if (response_cache.cached_text) {
        free(response_cache.cached_text);
        response_cache.cached_text = NULL;
    }

    response_cache.is_valid = false;
}