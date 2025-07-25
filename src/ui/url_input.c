#include "ui_components.h"
#include <string.h>

void RenderUrlInput(Rectangle bounds, char* url_buffer, int buffer_size, GruvboxTheme* theme) {
    if (!url_buffer || !theme || buffer_size <= 0) return;

    static bool url_input_active = false;
    static int cursor_position = 0;
    static float cursor_blink_timer = 0.0f;

    cursor_blink_timer += GetFrameTime();
    if (cursor_blink_timer > 1.0f) {
        cursor_blink_timer = 0.0f;
    }

    bool mouse_in_bounds = CheckCollisionPointRec(GetMousePosition(), bounds);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        url_input_active = mouse_in_bounds;
        if (url_input_active) {

            Vector2 mouse_pos = GetMousePosition();
            int url_len = strlen(url_buffer);
            cursor_position = url_len;

            for (int i = 0; i <= url_len; i++) {
                char temp_char = url_buffer[i];
                url_buffer[i] = '\0';
                Vector2 partial_size = MeasureTextEx(GetCodeFont(), url_buffer, FONT_SIZE_NORMAL, 1);
                url_buffer[i] = temp_char;

                if (bounds.x + PADDING_MEDIUM + partial_size.x > mouse_pos.x) {
                    cursor_position = i;
                    break;
                }
            }
        }
    }

    if (url_input_active) {

        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126) {
                int url_len = strlen(url_buffer);
                if (url_len < buffer_size - 1) {

                    memmove(&url_buffer[cursor_position + 1], &url_buffer[cursor_position],
                           url_len - cursor_position + 1);
                    url_buffer[cursor_position] = (char)key;
                    cursor_position++;
                }
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && cursor_position > 0) {
            int url_len = strlen(url_buffer);
            memmove(&url_buffer[cursor_position - 1], &url_buffer[cursor_position],
                   url_len - cursor_position + 1);
            cursor_position--;
        }

        if (IsKeyPressed(KEY_DELETE)) {
            int url_len = strlen(url_buffer);
            if (cursor_position < url_len) {
                memmove(&url_buffer[cursor_position], &url_buffer[cursor_position + 1],
                       url_len - cursor_position);
            }
        }

        if (IsKeyPressed(KEY_LEFT) && cursor_position > 0) {
            cursor_position--;
        }

        if (IsKeyPressed(KEY_RIGHT) && cursor_position < (int)strlen(url_buffer)) {
            cursor_position++;
        }

        if (IsKeyPressed(KEY_HOME)) {
            cursor_position = 0;
        }

        if (IsKeyPressed(KEY_END)) {
            cursor_position = strlen(url_buffer);
        }

        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_A)) {
            cursor_position = strlen(url_buffer);
        }

        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
            const char* clipboard = GetClipboardText();
            if (clipboard) {
                int clipboard_len = strlen(clipboard);
                int url_len = strlen(url_buffer);
                int available_space = buffer_size - url_len - 1;

                if (clipboard_len <= available_space) {
                    memmove(&url_buffer[cursor_position + clipboard_len], &url_buffer[cursor_position],
                           url_len - cursor_position + 1);
                    memcpy(&url_buffer[cursor_position], clipboard, clipboard_len);
                    cursor_position += clipboard_len;
                }
            }
        }
    }

    ValidationResult url_validation = ValidateUrlInput(url_buffer);
    bool is_valid_url = url_validation.is_valid;

    Color bg_color = url_input_active ? theme->bg2 : theme->bg1;
    Color border_color;
    if (url_input_active) {
        border_color = is_valid_url ? theme->blue : theme->red;
    } else {
        border_color = is_valid_url ? theme->gray : theme->red;
    }

    DrawRectangleRec(bounds, bg_color);
    DrawRectangleLinesEx(bounds, 2, border_color);

    if (strlen(url_buffer) == 0) {
        const char* placeholder = "Enter URL (e.g., https://api.example.com)";
        Vector2 placeholder_size = MeasureTextEx(GetCodeFont(), placeholder, FONT_SIZE_NORMAL, 1);
        Vector2 placeholder_pos = {
            bounds.x + PADDING_MEDIUM,
            bounds.y + (bounds.height - placeholder_size.y) / 2
        };
        DrawTextEx(GetCodeFont(), placeholder, placeholder_pos, FONT_SIZE_NORMAL, 1, theme->gray);
    } else {

        Vector2 text_pos = {
            bounds.x + PADDING_MEDIUM,
            bounds.y + (bounds.height - FONT_SIZE_NORMAL) / 2
        };

        Vector2 text_size = MeasureTextEx(GetCodeFont(), url_buffer, FONT_SIZE_NORMAL, 1);
        float available_width = bounds.width - (2 * PADDING_MEDIUM) - 30;
        float text_offset = 0;

        if (text_size.x > available_width) {

            char temp_char = url_buffer[cursor_position];
            url_buffer[cursor_position] = '\0';
            Vector2 cursor_text_size = MeasureTextEx(GetCodeFont(), url_buffer, FONT_SIZE_NORMAL, 1);
            url_buffer[cursor_position] = temp_char;

            if (cursor_text_size.x - text_offset > available_width - 20) {
                text_offset = cursor_text_size.x - available_width + 20;
            } else if (cursor_text_size.x - text_offset < 20) {
                text_offset = cursor_text_size.x - 20;
                if (text_offset < 0) text_offset = 0;
            }
        }

        BeginScissorMode((int)bounds.x + PADDING_MEDIUM, (int)bounds.y,
                        (int)(bounds.width - 2 * PADDING_MEDIUM - 30), (int)bounds.height);

        Vector2 adjusted_text_pos = {text_pos.x - text_offset, text_pos.y};
        Color text_color = is_valid_url ? theme->fg0 : theme->red;
        DrawTextEx(GetCodeFont(), url_buffer, adjusted_text_pos, FONT_SIZE_NORMAL, 1, text_color);

        if (url_input_active && cursor_blink_timer < 0.5f) {
            char temp_char = url_buffer[cursor_position];
            url_buffer[cursor_position] = '\0';
            Vector2 cursor_text_size = MeasureTextEx(GetCodeFont(), url_buffer, FONT_SIZE_NORMAL, 1);
            url_buffer[cursor_position] = temp_char;

            float cursor_x = adjusted_text_pos.x + cursor_text_size.x;
            float cursor_y = text_pos.y;
            DrawRectangle((int)cursor_x, (int)cursor_y, 2, FONT_SIZE_NORMAL, theme->fg0);
        }

        EndScissorMode();
    }

    if (strlen(url_buffer) > 0) {
        Rectangle indicator_bounds = {
            bounds.x + bounds.width - 25,
            bounds.y + 5,
            15,
            bounds.height - 10
        };

        if (is_valid_url) {
            DrawRectangleRec(indicator_bounds, theme->green);

            Vector2 check_start = {indicator_bounds.x + 3, indicator_bounds.y + indicator_bounds.height / 2};
            Vector2 check_mid = {indicator_bounds.x + 6, indicator_bounds.y + indicator_bounds.height - 4};
            Vector2 check_end = {indicator_bounds.x + 12, indicator_bounds.y + 3};
            DrawLineEx(check_start, check_mid, 2, theme->bg0);
            DrawLineEx(check_mid, check_end, 2, theme->bg0);
        } else {
            DrawRectangleRec(indicator_bounds, theme->red);

            DrawLineEx(
                (Vector2){indicator_bounds.x + 3, indicator_bounds.y + 3},
                (Vector2){indicator_bounds.x + 12, indicator_bounds.y + indicator_bounds.height - 3},
                2, theme->bg0
            );
            DrawLineEx(
                (Vector2){indicator_bounds.x + 12, indicator_bounds.y + 3},
                (Vector2){indicator_bounds.x + 3, indicator_bounds.y + indicator_bounds.height - 3},
                2, theme->bg0
            );
        }
    }
}