#include "ui_components.h"
#include "request_state.h"
#include "http_client.h"
#include "json_processor.h"
#include "memory_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "ui/method_selector.c"
#include "ui/url_input.c"
#include "ui/response_viewer.c"
#include "ui/ui_utils.c"
#include "ui/validation.c"
#include "ui/error_handling.c"

void RenderHeadersEditor(Rectangle bounds, char** headers, int* header_count, GruvboxTheme* theme) {
    if (!headers || !header_count || !theme) return;

    static int active_header_key = -1;
    static int active_header_value = -1;
    static int cursor_position = 0;
    static float cursor_blink_timer = 0.0f;
    static char temp_key[MAX_HEADER_LENGTH] = {0};
    static char temp_value[MAX_HEADER_LENGTH] = {0};
    static bool adding_new_header = false;
    static bool editing_key_field = true;

    cursor_blink_timer += GetFrameTime();
    if (cursor_blink_timer > 1.0f) {
        cursor_blink_timer = 0.0f;
    }

    const float row_height = 35;
    const float key_width = bounds.width * 0.4f;
    const float value_width = bounds.width * 0.5f;
    const float button_width = bounds.width * 0.1f;

    Vector2 title_pos = {bounds.x, bounds.y};
    DrawTextEx(GetUIFont(), "Headers:", title_pos, FONT_SIZE_NORMAL, 1, theme->fg0);

    float current_y = bounds.y + 25;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse_pos = GetMousePosition();
        bool clicked_in_editor = CheckCollisionPointRec(mouse_pos, bounds);
        if (!clicked_in_editor) {
            active_header_key = -1;
            active_header_value = -1;
            adding_new_header = false;
        }
    }

    for (int i = 0; i < *header_count && i < MAX_HEADERS; i++) {
        Rectangle key_bounds = {bounds.x, current_y + 5, key_width - 5, 25};
        Rectangle value_bounds = {bounds.x + key_width, current_y + 5, value_width - 5, 25};
        Rectangle remove_bounds = {bounds.x + key_width + value_width, current_y + 5, button_width - 5, 25};

        char header_key[MAX_HEADER_LENGTH] = {0};
        char header_value[MAX_HEADER_LENGTH] = {0};

        if (headers[i]) {
            char* colon_pos = strchr(headers[i], ':');
            if (colon_pos) {
                size_t key_len = colon_pos - headers[i];
                if (key_len < MAX_HEADER_LENGTH - 1) {
                    strncpy(header_key, headers[i], key_len);
                    header_key[key_len] = '\0';

                    const char* value_start = colon_pos + 1;
                    while (*value_start == ' ') value_start++;

                    if (strlen(value_start) < MAX_HEADER_LENGTH - 1) {
                        strcpy(header_value, value_start);
                    }
                }
            }
        }

        bool key_clicked = CheckCollisionPointRec(GetMousePosition(), key_bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool value_clicked = CheckCollisionPointRec(GetMousePosition(), value_bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool remove_clicked = CheckCollisionPointRec(GetMousePosition(), remove_bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        if (key_clicked) {
            active_header_key = i;
            active_header_value = -1;
            adding_new_header = false;
            strcpy(temp_key, header_key);
            cursor_position = strlen(temp_key);
        } else if (value_clicked) {
            active_header_value = i;
            active_header_key = -1;
            adding_new_header = false;
            strcpy(temp_value, header_value);
            cursor_position = strlen(temp_value);
        }

        if (remove_clicked) {

            if (headers[i]) {
                free(headers[i]);
            }

            for (int j = i; j < *header_count - 1; j++) {
                headers[j] = headers[j + 1];
            }
            headers[*header_count - 1] = NULL;
            (*header_count)--;

            active_header_key = -1;
            active_header_value = -1;
            adding_new_header = false;
            continue;
        }

        Color key_bg = (active_header_key == i) ? theme->bg2 : theme->bg1;
        Color key_border = (active_header_key == i) ? theme->blue : theme->gray;
        DrawRectangleRec(key_bounds, key_bg);
        DrawRectangleLinesEx(key_bounds, 1, key_border);

        const char* key_text = (active_header_key == i) ? temp_key : header_key;
        Vector2 key_text_pos = {key_bounds.x + PADDING_SMALL, key_bounds.y + (key_bounds.height - FONT_SIZE_SMALL) / 2};
        DrawTextEx(GetCodeFont(), key_text, key_text_pos, FONT_SIZE_SMALL, 1, theme->fg0);

        if (active_header_key == i && cursor_blink_timer < 0.5f) {
            char temp_char = temp_key[cursor_position];
            temp_key[cursor_position] = '\0';
            Vector2 cursor_text_size = MeasureTextEx(GetCodeFont(), temp_key, FONT_SIZE_SMALL, 1);
            temp_key[cursor_position] = temp_char;

            float cursor_x = key_text_pos.x + cursor_text_size.x;
            DrawRectangle((int)cursor_x, (int)key_text_pos.y, 2, FONT_SIZE_SMALL, theme->fg0);
        }

        Color value_bg = (active_header_value == i) ? theme->bg2 : theme->bg1;
        Color value_border = (active_header_value == i) ? theme->blue : theme->gray;
        DrawRectangleRec(value_bounds, value_bg);
        DrawRectangleLinesEx(value_bounds, 1, value_border);

        const char* value_text = (active_header_value == i) ? temp_value : header_value;
        Vector2 value_text_pos = {value_bounds.x + PADDING_SMALL, value_bounds.y + (value_bounds.height - FONT_SIZE_SMALL) / 2};
        DrawTextEx(GetCodeFont(), value_text, value_text_pos, FONT_SIZE_SMALL, 1, theme->fg0);

        if (active_header_value == i && cursor_blink_timer < 0.5f) {
            char temp_char = temp_value[cursor_position];
            temp_value[cursor_position] = '\0';
            Vector2 cursor_text_size = MeasureTextEx(GetCodeFont(), temp_value, FONT_SIZE_SMALL, 1);
            temp_value[cursor_position] = temp_char;

            float cursor_x = value_text_pos.x + cursor_text_size.x;
            DrawRectangle((int)cursor_x, (int)value_text_pos.y, 2, FONT_SIZE_SMALL, theme->fg0);
        }

        bool remove_hovered = CheckCollisionPointRec(GetMousePosition(), remove_bounds);
        Color remove_bg = remove_hovered ? theme->red : theme->bg1;
        DrawRectangleRec(remove_bounds, remove_bg);
        DrawRectangleLinesEx(remove_bounds, 1, theme->red);

        Vector2 remove_text_pos = {remove_bounds.x + (remove_bounds.width - 8) / 2, remove_bounds.y + (remove_bounds.height - FONT_SIZE_SMALL) / 2};
        DrawTextEx(GetUIFont(), "×", remove_text_pos, FONT_SIZE_SMALL, 1,
                  remove_hovered ? theme->fg0 : theme->red);

        current_y += row_height;
    }

    if (*header_count < MAX_HEADERS) {
        Rectangle add_key_bounds = {bounds.x, current_y + 5, key_width - 5, 25};
        Rectangle add_value_bounds = {bounds.x + key_width, current_y + 5, value_width - 5, 25};
        Rectangle add_button_bounds = {bounds.x + key_width + value_width, current_y + 5, button_width - 5, 25};

        bool add_key_clicked = CheckCollisionPointRec(GetMousePosition(), add_key_bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool add_value_clicked = CheckCollisionPointRec(GetMousePosition(), add_value_bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool add_button_clicked = CheckCollisionPointRec(GetMousePosition(), add_button_bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        if (add_key_clicked) {
            active_header_key = -1;
            active_header_value = -1;
            adding_new_header = true;
            editing_key_field = true;
            cursor_position = strlen(temp_key);
        } else if (add_value_clicked) {
            active_header_key = -1;
            active_header_value = -1;
            adding_new_header = true;
            editing_key_field = false;
            cursor_position = strlen(temp_value);
        }

        if (add_button_clicked && strlen(temp_key) > 0 && strlen(temp_value) > 0) {

            size_t header_len = strlen(temp_key) + strlen(temp_value) + 3;
            headers[*header_count] = malloc(header_len);
            if (headers[*header_count]) {
                snprintf(headers[*header_count], header_len, "%s: %s", temp_key, temp_value);
                (*header_count)++;

                temp_key[0] = '\0';
                temp_value[0] = '\0';
                adding_new_header = false;
                editing_key_field = true;
            }
        }

        Color add_key_bg = (adding_new_header && editing_key_field) ? theme->bg2 : theme->bg1;
        Color add_key_border = (adding_new_header && editing_key_field) ? theme->green : theme->gray;
        DrawRectangleRec(add_key_bounds, add_key_bg);
        DrawRectangleLinesEx(add_key_bounds, 1, add_key_border);

        if (strlen(temp_key) == 0) {
            Vector2 placeholder_pos = {add_key_bounds.x + PADDING_SMALL, add_key_bounds.y + (add_key_bounds.height - FONT_SIZE_SMALL) / 2};
            DrawTextEx(GetCodeFont(), "Header name", placeholder_pos, FONT_SIZE_SMALL, 1, theme->gray);
        } else {
            Vector2 key_text_pos = {add_key_bounds.x + PADDING_SMALL, add_key_bounds.y + (add_key_bounds.height - FONT_SIZE_SMALL) / 2};
            DrawTextEx(GetCodeFont(), temp_key, key_text_pos, FONT_SIZE_SMALL, 1, theme->fg0);

            if (adding_new_header && editing_key_field && cursor_blink_timer < 0.5f) {
                char temp_char = temp_key[cursor_position];
                temp_key[cursor_position] = '\0';
                Vector2 cursor_text_size = MeasureTextEx(GetCodeFont(), temp_key, FONT_SIZE_SMALL, 1);
                temp_key[cursor_position] = temp_char;

                float cursor_x = key_text_pos.x + cursor_text_size.x;
                DrawRectangle((int)cursor_x, (int)key_text_pos.y, 2, FONT_SIZE_SMALL, theme->fg0);
            }
        }

        Color add_value_bg = (adding_new_header && !editing_key_field) ? theme->bg2 : theme->bg1;
        Color add_value_border = (adding_new_header && !editing_key_field) ? theme->green : theme->gray;
        DrawRectangleRec(add_value_bounds, add_value_bg);
        DrawRectangleLinesEx(add_value_bounds, 1, add_value_border);

        if (strlen(temp_value) == 0) {
            Vector2 placeholder_pos = {add_value_bounds.x + PADDING_SMALL, add_value_bounds.y + (add_value_bounds.height - FONT_SIZE_SMALL) / 2};
            DrawTextEx(GetCodeFont(), "Header value", placeholder_pos, FONT_SIZE_SMALL, 1, theme->gray);
        } else {
            Vector2 value_text_pos = {add_value_bounds.x + PADDING_SMALL, add_value_bounds.y + (add_value_bounds.height - FONT_SIZE_SMALL) / 2};
            DrawTextEx(GetCodeFont(), temp_value, value_text_pos, FONT_SIZE_SMALL, 1, theme->fg0);

            if (adding_new_header && !editing_key_field && cursor_blink_timer < 0.5f) {
                char temp_char = temp_value[cursor_position];
                temp_value[cursor_position] = '\0';
                Vector2 cursor_text_size = MeasureTextEx(GetCodeFont(), temp_value, FONT_SIZE_SMALL, 1);
                temp_value[cursor_position] = temp_char;

                float cursor_x = value_text_pos.x + cursor_text_size.x;
                DrawRectangle((int)cursor_x, (int)value_text_pos.y, 2, FONT_SIZE_SMALL, theme->fg0);
            }
        }

        bool add_button_hovered = CheckCollisionPointRec(GetMousePosition(), add_button_bounds);
        bool can_add = strlen(temp_key) > 0 && strlen(temp_value) > 0;
        Color add_button_bg = can_add ? (add_button_hovered ? theme->green : BlendColors(theme->bg1, theme->green, 0.3f)) : theme->bg1;
        DrawRectangleRec(add_button_bounds, add_button_bg);
        DrawRectangleLinesEx(add_button_bounds, 1, can_add ? theme->green : theme->gray);

        Vector2 add_text_pos = {add_button_bounds.x + (add_button_bounds.width - 8) / 2, add_button_bounds.y + (add_button_bounds.height - FONT_SIZE_SMALL) / 2};
        Color add_text_color = can_add ? theme->fg0 : theme->gray;
        DrawTextEx(GetUIFont(), "+", add_text_pos, FONT_SIZE_SMALL, 1, add_text_color);
    }

    if (active_header_key >= 0 || active_header_value >= 0 || adding_new_header) {
        char* target_buffer = NULL;
        int max_len = MAX_HEADER_LENGTH - 1;

        if (active_header_key >= 0) {
            target_buffer = temp_key;
        } else if (active_header_value >= 0) {
            target_buffer = temp_value;
        } else if (adding_new_header) {
            target_buffer = editing_key_field ? temp_key : temp_value;
        }

        if (target_buffer) {

            int key = GetCharPressed();
            while (key > 0) {
                if (key >= 32 && key <= 126) {
                    int len = strlen(target_buffer);
                    if (len < max_len && cursor_position <= len) {
                        memmove(&target_buffer[cursor_position + 1], &target_buffer[cursor_position],
                               len - cursor_position + 1);
                        target_buffer[cursor_position] = (char)key;
                        cursor_position++;
                    }
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) && cursor_position > 0) {
                int len = strlen(target_buffer);
                memmove(&target_buffer[cursor_position - 1], &target_buffer[cursor_position],
                       len - cursor_position + 1);
                cursor_position--;
            }

            if (IsKeyPressed(KEY_DELETE)) {
                int len = strlen(target_buffer);
                if (cursor_position < len) {
                    memmove(&target_buffer[cursor_position], &target_buffer[cursor_position + 1],
                           len - cursor_position);
                }
            }

            if (IsKeyPressed(KEY_LEFT) && cursor_position > 0) {
                cursor_position--;
            }

            if (IsKeyPressed(KEY_RIGHT) && cursor_position < (int)strlen(target_buffer)) {
                cursor_position++;
            }

            if (IsKeyPressed(KEY_HOME)) {
                cursor_position = 0;
            }

            if (IsKeyPressed(KEY_END)) {
                cursor_position = strlen(target_buffer);
            }

            if (adding_new_header && IsKeyPressed(KEY_TAB)) {
                editing_key_field = !editing_key_field;
                cursor_position = strlen(editing_key_field ? temp_key : temp_value);
            }

            if (adding_new_header && IsKeyPressed(KEY_ENTER) && strlen(temp_key) > 0 && strlen(temp_value) > 0) {
                size_t header_len = strlen(temp_key) + strlen(temp_value) + 3;
                headers[*header_count] = malloc(header_len);
                if (headers[*header_count]) {
                    snprintf(headers[*header_count], header_len, "%s: %s", temp_key, temp_value);
                    (*header_count)++;

                    temp_key[0] = '\0';
                    temp_value[0] = '\0';
                    adding_new_header = false;
                    editing_key_field = true;
                    cursor_position = 0;
                }
            }

            if (active_header_key >= 0 || active_header_value >= 0) {
                if (IsKeyPressed(KEY_ENTER)) {

                    int header_index = (active_header_key >= 0) ? active_header_key : active_header_value;

                    char current_key[MAX_HEADER_LENGTH] = {0};
                    char current_value[MAX_HEADER_LENGTH] = {0};

                    if (headers[header_index]) {
                        char* colon_pos = strchr(headers[header_index], ':');
                        if (colon_pos) {
                            size_t key_len = colon_pos - headers[header_index];
                            if (key_len < MAX_HEADER_LENGTH - 1) {
                                strncpy(current_key, headers[header_index], key_len);
                                current_key[key_len] = '\0';

                                const char* value_start = colon_pos + 1;
                                while (*value_start == ' ') value_start++;

                                if (strlen(value_start) < MAX_HEADER_LENGTH - 1) {
                                    strcpy(current_value, value_start);
                                }
                            }
                        }
                    }

                    const char* new_key = (active_header_key >= 0) ? temp_key : current_key;
                    const char* new_value = (active_header_value >= 0) ? temp_value : current_value;

                    if (strlen(new_key) > 0 && strlen(new_value) > 0) {
                        free(headers[header_index]);
                        size_t header_len = strlen(new_key) + strlen(new_value) + 3;
                        headers[header_index] = malloc(header_len);
                        if (headers[header_index]) {
                            snprintf(headers[header_index], header_len, "%s: %s", new_key, new_value);
                        }
                    }

                    active_header_key = -1;
                    active_header_value = -1;
                }
            }
        }
    }
}

void RenderBodyEditor(Rectangle bounds, char* body_buffer, int buffer_size, GruvboxTheme* theme) {
    if (!body_buffer || !theme || buffer_size <= 0) return;

    DrawRectangleRec(bounds, theme->bg1);
    DrawRectangleLinesEx(bounds, 1, theme->gray);

    Vector2 text_pos = {bounds.x + PADDING_SMALL, bounds.y + PADDING_SMALL};
    DrawTextEx(GetCodeFont(), body_buffer, text_pos, FONT_SIZE_SMALL, 1, theme->fg0);
}