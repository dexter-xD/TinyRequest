#include "json_processor.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static Color GetTokenColor(JsonTokenType type, GruvboxTheme* theme) {
    switch (type) {
        case JSON_TOKEN_KEY:
            return theme->blue;
        case JSON_TOKEN_STRING:
            return theme->purple;
        case JSON_TOKEN_NUMBER:
            return theme->aqua;
        case JSON_TOKEN_BOOLEAN:
            return theme->orange;
        case JSON_TOKEN_NULL:
            return theme->gray;
        case JSON_TOKEN_BRACE:
        case JSON_TOKEN_BRACKET:
        case JSON_TOKEN_COMMA:
        case JSON_TOKEN_COLON:
            return theme->fg1;
        case JSON_TOKEN_WHITESPACE:
            return theme->fg0;
        default:
            return theme->fg0;
    }
}

void RenderJsonWithHighlighting(Rectangle bounds, const char* json, GruvboxTheme* theme) {
    if (!json || !theme) return;

    BeginScissorMode((int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height);

    char* formatted_json = FormatJson(json);
    const char* display_json = formatted_json ? formatted_json : json;
    int json_len = strlen(display_json);

    static float json_scroll_offset = 0.0f;
    static const char* last_json_content = NULL;

    if (last_json_content != display_json) {
        json_scroll_offset = 0.0f;
        last_json_content = display_json;
    }

    if (CheckCollisionPointRec(GetMousePosition(), bounds)) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            json_scroll_offset -= wheel * (FONT_SIZE_SMALL + 3) * 5;
            if (json_scroll_offset < 0) json_scroll_offset = 0;
        }
    }

    int total_lines = 1;
    for (const char* p = display_json; *p; p++) {
        if (*p == '\n') total_lines++;
    }

    const float line_height = FONT_SIZE_SMALL + 3;
    int visible_lines = (int)(bounds.height / line_height);
    float max_scroll = (total_lines - visible_lines + 2) * line_height;
    if (max_scroll < 0) max_scroll = 0;
    if (json_scroll_offset > max_scroll) json_scroll_offset = max_scroll;

    Vector2 current_pos = {bounds.x + PADDING_SMALL, bounds.y + PADDING_SMALL - json_scroll_offset};
    const char* line_start = display_json;
    const char* current = display_json;
    int line_number = 0;

    while (*current) {
        if (*current == '\n') {
            if (current_pos.y >= bounds.y - 20 && current_pos.y <= bounds.y + bounds.height + 20) {
                if (current > line_start) {
                    size_t line_len = current - line_start;
                    if (line_len > 0 && line_len < 2048) {
                        char* line_buffer = malloc(line_len + 1);
                        if (line_buffer) {
                            strncpy(line_buffer, line_start, line_len);
                            line_buffer[line_len] = '\0';

                            RenderJsonLine(current_pos, line_buffer, theme);
                            free(line_buffer);
                        }
                    }
                }
            }

            current_pos.y += line_height;
            current_pos.x = bounds.x + PADDING_SMALL;
            line_number++;

            current++;
            line_start = current;
        } else {
            current++;
        }
    }
    
    if (current > line_start) {
        if (current_pos.y >= bounds.y - 20 && current_pos.y <= bounds.y + bounds.height + 20) {
            size_t line_len = current - line_start;
            if (line_len > 0 && line_len < 2048) {
                char* line_buffer = malloc(line_len + 1);
                if (line_buffer) {
                    strncpy(line_buffer, line_start, line_len);
                    line_buffer[line_len] = '\0';

                    RenderJsonLine(current_pos, line_buffer, theme);
                    free(line_buffer);
                }
            }
        }
    }

    if (total_lines > visible_lines) {
        const float scrollbar_width = 12;
        Rectangle scrollbar_track = {
            bounds.x + bounds.width - scrollbar_width,
            bounds.y,
            scrollbar_width,
            bounds.height
        };

        DrawRectangleRec(scrollbar_track, theme->bg2);
        DrawRectangleLinesEx(scrollbar_track, 1, theme->gray);

        float thumb_ratio = (float)visible_lines / total_lines;
        if (thumb_ratio > 1.0f) thumb_ratio = 1.0f;

        float thumb_height = scrollbar_track.height * thumb_ratio;
        if (thumb_height < 20) thumb_height = 20;

        float thumb_y = scrollbar_track.y + (json_scroll_offset / max_scroll) * (scrollbar_track.height - thumb_height);

        Rectangle scrollbar_thumb = {
            scrollbar_track.x + 2,
            thumb_y,
            scrollbar_width - 4,
            thumb_height
        };

        static bool dragging_scrollbar = false;
        static float drag_offset = 0;

        bool thumb_hovered = CheckCollisionPointRec(GetMousePosition(), scrollbar_thumb);
        bool track_clicked = CheckCollisionPointRec(GetMousePosition(), scrollbar_track) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        if (thumb_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            dragging_scrollbar = true;
            drag_offset = GetMousePosition().y - thumb_y;
        }

        if (dragging_scrollbar) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                float new_thumb_y = GetMousePosition().y - drag_offset;
                float scroll_ratio = (new_thumb_y - scrollbar_track.y) / (scrollbar_track.height - thumb_height);
                if (scroll_ratio < 0) scroll_ratio = 0;
                if (scroll_ratio > 1) scroll_ratio = 1;
                json_scroll_offset = scroll_ratio * max_scroll;
            } else {
                dragging_scrollbar = false;
            }
        } else if (track_clicked && !thumb_hovered) {

            float click_ratio = (GetMousePosition().y - scrollbar_track.y) / scrollbar_track.height;
            if (click_ratio < 0) click_ratio = 0;
            if (click_ratio > 1) click_ratio = 1;
            json_scroll_offset = click_ratio * max_scroll;
        }

        Color thumb_color = (thumb_hovered || dragging_scrollbar) ? theme->fg1 : theme->gray;
        DrawRectangleRec(scrollbar_thumb, thumb_color);
    }

    EndScissorMode();

    if (formatted_json) {
        free(formatted_json);
    }
}

void RenderJsonLine(Vector2 position, const char* line, GruvboxTheme* theme) {
    if (!line || !theme) return;

    Vector2 current_pos = position;
    int len = strlen(line);
    bool in_string = false;
    bool in_key = false;
    bool escaped = false;

    for (int i = 0; i < len; i++) {
        char c = line[i];
        Color text_color = theme->fg0;

        if (in_string) {
            if (escaped) {
                text_color = theme->orange;
                escaped = false;
            } else if (c == '\\') {
                text_color = theme->orange;
                escaped = true;
            } else if (c == '"') {
                text_color = in_key ? theme->blue : theme->purple;
                in_string = false;
                in_key = false;
            } else {
                text_color = in_key ? theme->blue : theme->purple;
            }
        } else {
            switch (c) {
                case '"':
                    in_string = true;

                    int j = i + 1;
                    while (j < len && line[j] != '"' && line[j] != '\n') j++;
                    if (j < len && line[j] == '"') {
                        j++;
                        while (j < len && (line[j] == ' ' || line[j] == '\t')) j++;
                        if (j < len && line[j] == ':') {
                            in_key = true;
                        }
                    }
                    text_color = in_key ? theme->blue : theme->purple;
                    break;
                case '{':
                case '}':
                case '[':
                case ']':
                    text_color = theme->fg1;
                    break;
                case ':':
                case ',':
                    text_color = theme->fg1;
                    break;
                case ' ':
                case '\t':
                    text_color = theme->fg0;
                    break;
                default:

                    if (isdigit(c) || c == '-' || c == '+' || c == '.') {
                        text_color = theme->aqua;
                    }

                    else if (strncmp(&line[i], "true", 4) == 0 &&
                             (i + 4 >= len || !isalnum(line[i + 4]))) {

                        DrawTextEx(GetCodeFont(), "true", current_pos, FONT_SIZE_SMALL, 1, theme->orange);
                        Vector2 word_size = MeasureTextEx(GetCodeFont(), "true", FONT_SIZE_SMALL, 1);
                        current_pos.x += word_size.x;
                        i += 3;
                        continue;
                    }
                    else if (strncmp(&line[i], "false", 5) == 0 &&
                             (i + 5 >= len || !isalnum(line[i + 5]))) {

                        DrawTextEx(GetCodeFont(), "false", current_pos, FONT_SIZE_SMALL, 1, theme->orange);
                        Vector2 word_size = MeasureTextEx(GetCodeFont(), "false", FONT_SIZE_SMALL, 1);
                        current_pos.x += word_size.x;
                        i += 4;
                        continue;
                    }
                    else if (strncmp(&line[i], "null", 4) == 0 &&
                             (i + 4 >= len || !isalnum(line[i + 4]))) {

                        DrawTextEx(GetCodeFont(), "null", current_pos, FONT_SIZE_SMALL, 1, theme->gray);
                        Vector2 word_size = MeasureTextEx(GetCodeFont(), "null", FONT_SIZE_SMALL, 1);
                        current_pos.x += word_size.x;
                        i += 3;
                        continue;
                    }
                    else {
                        text_color = theme->fg0;
                    }
                    break;
            }
        }

        char char_str[2] = {c, '\0'};
        DrawTextEx(GetCodeFont(), char_str, current_pos, FONT_SIZE_SMALL, 1, text_color);

        Vector2 char_size = MeasureTextEx(GetCodeFont(), char_str, FONT_SIZE_SMALL, 1);
        current_pos.x += char_size.x;
    }
}

void RenderTokensChunked(Rectangle bounds, JsonToken* tokens, int token_count, GruvboxTheme* theme) {
    Vector2 current_pos = {bounds.x + PADDING_SMALL, bounds.y + PADDING_SMALL};
    Vector2 start_pos = current_pos;

    const int CHUNK_SIZE = 100;

    for (int chunk_start = 0; chunk_start < token_count; chunk_start += CHUNK_SIZE) {
        int chunk_end = chunk_start + CHUNK_SIZE;
        if (chunk_end > token_count) chunk_end = token_count;

        for (int i = chunk_start; i < chunk_end; i++) {
            JsonToken* token = &tokens[i];

            if (current_pos.y > bounds.y + bounds.height + 50) {
                return;
            }

            Color token_color = GetTokenColor(token->type, theme);

            if (token->text) {
                for (int j = 0; j < token->length; j++) {
                    char c = token->text[j];

                    if (c == '\n') {
                        current_pos.x = start_pos.x;
                        current_pos.y += FONT_SIZE_SMALL + 2;
                    } else {

                        if (current_pos.y >= bounds.y - 20 && current_pos.y <= bounds.y + bounds.height + 20) {
                            char char_str[2] = {c, '\0'};
                            DrawTextEx(GetCodeFont(), char_str, current_pos, FONT_SIZE_SMALL, 1, token_color);
                        }

                        Vector2 char_size = MeasureTextEx(GetCodeFont(), &c, FONT_SIZE_SMALL, 1);
                        current_pos.x += char_size.x;

                        if (current_pos.x > bounds.x + bounds.width - 20) {
                            current_pos.x = start_pos.x;
                            current_pos.y += FONT_SIZE_SMALL + 2;
                        }
                    }
                }
            }
        }
    }
}

void RenderLargeJsonOptimized(Rectangle bounds, const char* json, GruvboxTheme* theme) {

    Vector2 current_pos = {bounds.x + PADDING_SMALL, bounds.y + PADDING_SMALL};
    const char* line_start = json;
    const char* current = json;

    while (*current) {
        if (*current == '\n' || current - json > 100000) {

            if (current_pos.y >= bounds.y - 20 && current_pos.y <= bounds.y + bounds.height + 20) {
                size_t line_len = current - line_start;
                if (line_len > 0 && line_len < 1024) {
                    char* line_buffer = malloc(line_len + 1);
                    if (line_buffer) {
                        strncpy(line_buffer, line_start, line_len);
                        line_buffer[line_len] = '\0';

                        DrawTextEx(GetCodeFont(), line_buffer, current_pos, FONT_SIZE_SMALL, 1, theme->fg0);
                        free(line_buffer);
                    }
                }
            }

            current_pos.y += FONT_SIZE_SMALL + 2;
            current_pos.x = bounds.x + PADDING_SMALL;

            if (current_pos.y > bounds.y + bounds.height + 50) {
                break;
            }

            if (*current == '\n') {
                current++;
                line_start = current;
            } else {
                break;
            }
        } else {
            current++;
        }
    }
}

void DrawJsonTextWithHighlighting(Vector2 position, const char* json, GruvboxTheme* theme) {
    if (!json || !theme) return;

    Vector2 current_pos = position;
    int len = strlen(json);
    bool in_string = false;
    bool in_key = false;
    bool escaped = false;

    for (int i = 0; i < len; i++) {
        char c = json[i];

        if (c == '\n') {
            current_pos.x = position.x;
            current_pos.y += FONT_SIZE_NORMAL;
            continue;
        }

        Color text_color = theme->fg0;

        if (in_string) {
            if (escaped) {
                text_color = theme->orange;
                escaped = false;
            } else if (c == '\\') {
                text_color = theme->orange;
                escaped = true;
            } else if (c == '"') {
                text_color = in_key ? theme->blue : theme->purple;
                in_string = false;
                in_key = false;
            } else {
                text_color = in_key ? theme->blue : theme->purple;
            }
        } else {
            switch (c) {
                case '"':
                    in_string = true;

                    int j = i + 1;
                    while (j < len && json[j] != '"' && json[j] != '\n') j++;
                    if (j < len && json[j] == '"') {
                        j++;
                        while (j < len && (json[j] == ' ' || json[j] == '\t')) j++;
                        if (j < len && json[j] == ':') {
                            in_key = true;
                        }
                    }
                    text_color = in_key ? theme->blue : theme->purple;
                    break;
                case '{':
                case '}':
                case '[':
                case ']':
                    text_color = theme->fg1;
                    break;
                case ':':
                case ',':
                    text_color = theme->fg1;
                    break;
                case ' ':
                case '\t':
                    text_color = theme->fg0;
                    break;
                default:

                    if (isdigit(c) || c == '-' || c == '+' || c == '.') {
                        text_color = theme->aqua;
                    }

                    else if (strncmp(&json[i], "true", 4) == 0 ||
                             strncmp(&json[i], "false", 5) == 0) {
                        text_color = theme->orange;
                    }
                    else if (strncmp(&json[i], "null", 4) == 0) {
                        text_color = theme->gray;
                    }
                    else {
                        text_color = theme->fg0;
                    }
                    break;
            }
        }

        char char_str[2] = {c, '\0'};
        DrawTextEx(GetCodeFont(), char_str, current_pos, FONT_SIZE_NORMAL, 1, text_color);

        Vector2 char_size = MeasureTextEx(GetCodeFont(), char_str, FONT_SIZE_NORMAL, 1);
        current_pos.x += char_size.x;
    }
}

void DrawJsonText(Vector2 position, const char* json, Font font, GruvboxTheme* theme) {

    DrawJsonTextWithHighlighting(position, json, theme);
}

Vector2 MeasureJsonText(const char* json, Font font) {
    if (!json) return (Vector2){0, 0};
    return MeasureTextEx(font, json, FONT_SIZE_NORMAL, 1);
}