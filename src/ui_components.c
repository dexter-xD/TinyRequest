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

char* ProcessBodyEscapeSequences(const char* input) {
    if (!input) return NULL;

    size_t input_len = strlen(input);
    char* output = malloc(input_len + 1); 
    if (!output) return NULL;

    size_t out_pos = 0;
    for (size_t i = 0; i < input_len; i++) {
        if (input[i] == '\\' && i + 1 < input_len) {
            switch (input[i + 1]) {
                case 'n':
                    output[out_pos++] = '\n';
                    i++; 
                    break;
                case 't':
                    output[out_pos++] = '\t';
                    i++; 
                    break;
                case 'r':
                    output[out_pos++] = '\r';
                    i++; 
                    break;
                case '\\':
                    output[out_pos++] = '\\';
                    i++; 
                    break;
                case '"':
                    output[out_pos++] = '"';
                    i++; 
                    break;
                default:

                    output[out_pos++] = input[i];
                    break;
            }
        } else {
            output[out_pos++] = input[i];
        }
    }

    output[out_pos] = '\0';
    return output;
}

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

    const float row_height = 40;
    const float key_width = bounds.width * 0.42f;
    const float value_width = bounds.width * 0.48f;
    const float button_width = bounds.width * 0.1f;

    float current_y = bounds.y;

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
        DrawRectangleRounded(key_bounds, 0.15f, 8, key_bg);
        DrawRectangleRoundedLines(key_bounds, 0.15f, 8, key_border);

        const char* key_text = (active_header_key == i) ? temp_key : header_key;
        Vector2 key_text_pos = {key_bounds.x + PADDING_MEDIUM, key_bounds.y + (key_bounds.height - FONT_SIZE_NORMAL) / 2};
        DrawTextEx(GetUIFont(), key_text, key_text_pos, FONT_SIZE_NORMAL, 1, theme->fg0);

        if (active_header_key == i && cursor_blink_timer < 0.5f) {
            char temp_char = temp_key[cursor_position];
            temp_key[cursor_position] = '\0';
            Vector2 cursor_text_size = MeasureTextEx(GetUIFont(), temp_key, FONT_SIZE_NORMAL, 1);
            temp_key[cursor_position] = temp_char;

            float cursor_x = key_text_pos.x + cursor_text_size.x;
            DrawRectangle((int)cursor_x, (int)key_text_pos.y, 2, FONT_SIZE_NORMAL, theme->fg0);
        }

        Color value_bg = (active_header_value == i) ? theme->bg2 : theme->bg1;
        Color value_border = (active_header_value == i) ? theme->blue : theme->gray;
        DrawRectangleRounded(value_bounds, 0.15f, 8, value_bg);
        DrawRectangleRoundedLines(value_bounds, 0.15f, 8, value_border);

        const char* value_text = (active_header_value == i) ? temp_value : header_value;
        Vector2 value_text_pos = {value_bounds.x + PADDING_MEDIUM, value_bounds.y + (value_bounds.height - FONT_SIZE_NORMAL) / 2};
        DrawTextEx(GetUIFont(), value_text, value_text_pos, FONT_SIZE_NORMAL, 1, theme->fg0);

        if (active_header_value == i && cursor_blink_timer < 0.5f) {
            char temp_char = temp_value[cursor_position];
            temp_value[cursor_position] = '\0';
            Vector2 cursor_text_size = MeasureTextEx(GetUIFont(), temp_value, FONT_SIZE_NORMAL, 1);
            temp_value[cursor_position] = temp_char;

            float cursor_x = value_text_pos.x + cursor_text_size.x;
            DrawRectangle((int)cursor_x, (int)value_text_pos.y, 2, FONT_SIZE_NORMAL, theme->fg0);
        }

        bool remove_hovered = CheckCollisionPointRec(GetMousePosition(), remove_bounds);
        Color remove_bg = remove_hovered ? theme->red : theme->bg1;
        Color remove_border = theme->red;

        DrawRectangleRounded(remove_bounds, 0.2f, 8, remove_bg);
        DrawRectangleRoundedLines(remove_bounds, 0.2f, 8, remove_border);

        Vector2 remove_center = {remove_bounds.x + remove_bounds.width / 2, 
                                remove_bounds.y + remove_bounds.height / 2};
        float x_size = 8;
        Color x_color = remove_hovered ? theme->fg0 : theme->red;

        float thickness = 1.5f;
        DrawLineEx((Vector2){remove_center.x - x_size/2, remove_center.y - x_size/2},
                   (Vector2){remove_center.x + x_size/2, remove_center.y + x_size/2}, thickness, x_color);
        DrawLineEx((Vector2){remove_center.x + x_size/2, remove_center.y - x_size/2},
                   (Vector2){remove_center.x - x_size/2, remove_center.y + x_size/2}, thickness, x_color);

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
        DrawRectangleRounded(add_key_bounds, 0.15f, 8, add_key_bg);
        DrawRectangleRoundedLines(add_key_bounds, 0.15f, 8, add_key_border);

        if (strlen(temp_key) == 0) {
            Vector2 placeholder_pos = {add_key_bounds.x + PADDING_MEDIUM, add_key_bounds.y + (add_key_bounds.height - FONT_SIZE_NORMAL) / 2};
            DrawTextEx(GetUIFont(), "Header name", placeholder_pos, FONT_SIZE_NORMAL, 1, Fade(theme->gray, 0.7f));
        } else {
            Vector2 key_text_pos = {add_key_bounds.x + PADDING_MEDIUM, add_key_bounds.y + (add_key_bounds.height - FONT_SIZE_NORMAL) / 2};
            DrawTextEx(GetUIFont(), temp_key, key_text_pos, FONT_SIZE_NORMAL, 1, theme->fg0);

            if (adding_new_header && editing_key_field && cursor_blink_timer < 0.5f) {
                char temp_char = temp_key[cursor_position];
                temp_key[cursor_position] = '\0';
                Vector2 cursor_text_size = MeasureTextEx(GetUIFont(), temp_key, FONT_SIZE_NORMAL, 1);
                temp_key[cursor_position] = temp_char;

                float cursor_x = key_text_pos.x + cursor_text_size.x;
                DrawRectangle((int)cursor_x, (int)key_text_pos.y, 2, FONT_SIZE_NORMAL, theme->fg0);
            }
        }

        Color add_value_bg = (adding_new_header && !editing_key_field) ? theme->bg2 : theme->bg1;
        Color add_value_border = (adding_new_header && !editing_key_field) ? theme->green : theme->gray;
        DrawRectangleRounded(add_value_bounds, 0.15f, 8, add_value_bg);
        DrawRectangleRoundedLines(add_value_bounds, 0.15f, 8, add_value_border);

        if (strlen(temp_value) == 0) {
            Vector2 placeholder_pos = {add_value_bounds.x + PADDING_MEDIUM, add_value_bounds.y + (add_value_bounds.height - FONT_SIZE_NORMAL) / 2};
            DrawTextEx(GetUIFont(), "Header value", placeholder_pos, FONT_SIZE_NORMAL, 1, Fade(theme->gray, 0.7f));
        } else {
            Vector2 value_text_pos = {add_value_bounds.x + PADDING_MEDIUM, add_value_bounds.y + (add_value_bounds.height - FONT_SIZE_NORMAL) / 2};
            DrawTextEx(GetUIFont(), temp_value, value_text_pos, FONT_SIZE_NORMAL, 1, theme->fg0);

            if (adding_new_header && !editing_key_field && cursor_blink_timer < 0.5f) {
                char temp_char = temp_value[cursor_position];
                temp_value[cursor_position] = '\0';
                Vector2 cursor_text_size = MeasureTextEx(GetUIFont(), temp_value, FONT_SIZE_NORMAL, 1);
                temp_value[cursor_position] = temp_char;

                float cursor_x = value_text_pos.x + cursor_text_size.x;
                DrawRectangle((int)cursor_x, (int)value_text_pos.y, 2, FONT_SIZE_NORMAL, theme->fg0);
            }
        }

        bool add_button_hovered = CheckCollisionPointRec(GetMousePosition(), add_button_bounds);
        bool can_add = strlen(temp_key) > 0 && strlen(temp_value) > 0;

        Color add_button_bg = can_add ? 
            (add_button_hovered ? theme->green : BlendColors(theme->bg1, theme->green, 0.4f)) : 
            theme->bg1;
        Color add_button_border = can_add ? theme->green : theme->gray;

        DrawRectangleRounded(add_button_bounds, 0.2f, 8, add_button_bg);
        DrawRectangleRoundedLines(add_button_bounds, 0.2f, 8, add_button_border);

        Vector2 center = {add_button_bounds.x + add_button_bounds.width / 2, 
                         add_button_bounds.y + add_button_bounds.height / 2};
        float icon_size = 10;
        Color icon_color = can_add ? theme->fg0 : theme->gray;

        DrawRectangle(center.x - icon_size/2, center.y - 1, icon_size, 2, icon_color);

        DrawRectangle(center.x - 1, center.y - icon_size/2, 2, icon_size, icon_color);
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

static bool body_type_dropdown_open = false;
static int body_type_hover_index = -1;

static const char* body_type_strings[] = {
    "JSON",
    "Text"
};

bool IsBodyTypeDropdownOpen(void) {
    return body_type_dropdown_open;
}

void RenderBodyTypeDropdownOverlay(Rectangle bounds, BodyType* selected_body_type, GruvboxTheme* theme) {
    if (!selected_body_type || !theme || !body_type_dropdown_open) return;

    Rectangle button_bounds = bounds;
    button_bounds.height = 30;
    body_type_hover_index = -1;

    for (int i = 0; i < BODY_TYPE_COUNT; i++) {
        Rectangle option_bounds = {
            bounds.x,
            bounds.y + button_bounds.height + (i * 30),
            bounds.width,
            30
        };
        bool option_hovered = CheckCollisionPointRec(GetMousePosition(), option_bounds);
        bool option_clicked = option_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        if (option_hovered) {
            body_type_hover_index = i;
        }

        if (option_clicked) {
            *selected_body_type = i;
            body_type_dropdown_open = false;
        }

        Color option_bg = theme->bg1;
        if (option_hovered) {
            option_bg = theme->bg2;
        } else if (i == *selected_body_type) {
            option_bg = BlendColors(theme->bg1, theme->blue, 0.2f);
        }

        DrawRectangleRec(option_bounds, option_bg);
        DrawRectangleLinesEx(option_bounds, 1, theme->gray);

        Vector2 option_text_size = MeasureTextEx(GetCodeFont(), body_type_strings[i], FONT_SIZE_NORMAL, 1);
        Vector2 option_text_pos = {
            option_bounds.x + PADDING_MEDIUM,
            option_bounds.y + (option_bounds.height - option_text_size.y) / 2
        };
        Color text_color = (i == *selected_body_type) ? theme->blue : theme->fg0;
        if (option_hovered) {
            text_color = theme->fg0;
        }
        DrawTextEx(GetCodeFont(), body_type_strings[i], option_text_pos, FONT_SIZE_NORMAL, 1, text_color);
    }
}

void RenderBodyTypeSelector(Rectangle bounds, BodyType* selected_body_type, GruvboxTheme* theme) {
    if (!selected_body_type || !theme) return;

    if (*selected_body_type < 0 || *selected_body_type >= BODY_TYPE_COUNT) {
        *selected_body_type = BODY_TYPE_JSON;
    }

    Rectangle button_bounds = bounds;
    button_bounds.height = 30;

    bool button_hovered = CheckCollisionPointRec(GetMousePosition(), button_bounds);
    bool button_pressed = button_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (button_pressed) {
        body_type_dropdown_open = !body_type_dropdown_open;
    }

    if (body_type_dropdown_open && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !button_hovered) {
        bool clicked_in_dropdown = false;
        if (body_type_dropdown_open) {
            Rectangle dropdown_bounds = {
                bounds.x,
                bounds.y + button_bounds.height,
                bounds.width,
                30 * BODY_TYPE_COUNT
            };
            clicked_in_dropdown = CheckCollisionPointRec(GetMousePosition(), dropdown_bounds);
        }
        if (!clicked_in_dropdown) {
            body_type_dropdown_open = false;
        }
    }

    Color button_bg = button_hovered ? theme->bg2 : theme->bg1;
    Color button_border = button_hovered ? theme->blue : theme->gray;

    DrawRectangleRec(button_bounds, button_bg);
    DrawRectangleLinesEx(button_bounds, 1, button_border);

    const char* selected_text = body_type_strings[*selected_body_type];
    Vector2 text_size = MeasureTextEx(GetCodeFont(), selected_text, FONT_SIZE_NORMAL, 1);
    Vector2 text_pos = {
        button_bounds.x + PADDING_MEDIUM,
        button_bounds.y + (button_bounds.height - text_size.y) / 2
    };
    DrawTextEx(GetCodeFont(), selected_text, text_pos, FONT_SIZE_NORMAL, 1, theme->fg0);

    Vector2 arrow_pos = {
        button_bounds.x + button_bounds.width - 20,
        button_bounds.y + button_bounds.height / 2
    };
    if (body_type_dropdown_open) {

        DrawTriangle(
            (Vector2){arrow_pos.x - 5, arrow_pos.y + 3},
            (Vector2){arrow_pos.x + 5, arrow_pos.y + 3},
            (Vector2){arrow_pos.x, arrow_pos.y - 3},
            theme->fg1
        );
    } else {

        DrawTriangle(
            (Vector2){arrow_pos.x - 5, arrow_pos.y - 3},
            (Vector2){arrow_pos.x + 5, arrow_pos.y - 3},
            (Vector2){arrow_pos.x, arrow_pos.y + 3},
            theme->fg1
        );
    }
}

void RenderBodyEditor(Rectangle bounds, char* body_buffer, int buffer_size, GruvboxTheme* theme, BodyType* body_type) {
    if (!body_buffer || !theme || buffer_size <= 0 || !body_type) return;

    DrawRectangleRec(bounds, theme->bg1);
    DrawRectangleLinesEx(bounds, 1, theme->gray);

    Rectangle body_type_bounds = {bounds.x + 10, bounds.y + 10, 100, 30};
    RenderBodyTypeSelector(body_type_bounds, body_type, theme);

    Rectangle text_area = {bounds.x + 15, bounds.y + 55, bounds.width - 30, bounds.height - 75};

    static bool text_active = false;
    static float cursor_blink_timer = 0.0f;

    cursor_blink_timer += GetFrameTime();
    if (cursor_blink_timer > 1.0f) {
        cursor_blink_timer = 0.0f;
    }

    if (CheckCollisionPointRec(GetMousePosition(), text_area) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        text_active = true;
    } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(GetMousePosition(), body_type_bounds)) {
        text_active = false;
    }

    if (text_active) {

        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126) { 
                int len = strlen(body_buffer);
                if (len < buffer_size - 1) {

                    if (key == 'n' && len > 0 && body_buffer[len - 1] == '\\') {

                        body_buffer[len - 1] = '\n';
                        body_buffer[len] = '\0';
                    } else if (key == 't' && len > 0 && body_buffer[len - 1] == '\\') {

                        body_buffer[len - 1] = '\t';
                        body_buffer[len] = '\0';
                    } else if (key == 'r' && len > 0 && body_buffer[len - 1] == '\\') {

                        body_buffer[len - 1] = '\r';
                        body_buffer[len] = '\0';
                    } else if (key == '"' && len > 0 && body_buffer[len - 1] == '\\') {

                        body_buffer[len - 1] = '"';
                        body_buffer[len] = '\0';
                    } else if (key == '\\' && len > 0 && body_buffer[len - 1] == '\\') {

                        body_buffer[len - 1] = '\\';
                        body_buffer[len] = '\0';
                    } else {

                        body_buffer[len] = (char)key;
                        body_buffer[len + 1] = '\0';
                    }
                }
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
            int len = strlen(body_buffer);
            if (len > 0) {
                body_buffer[len - 1] = '\0';
            }
        }

        if (IsKeyPressed(KEY_ENTER)) {
            int len = strlen(body_buffer);
            if (len < buffer_size - 1) {
                body_buffer[len] = '\n';
                body_buffer[len + 1] = '\0';
            }
        }

        if (IsKeyPressed(KEY_TAB) && *body_type == BODY_TYPE_JSON) {
            int len = strlen(body_buffer);
            if (len < buffer_size - 4) {
                strcat(body_buffer, "    "); 
            }
        }

        if (IsKeyPressed(KEY_V) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))) {
            const char* clipboard_text = GetClipboardText();
            if (clipboard_text && strlen(clipboard_text) > 0) {
                int current_len = strlen(body_buffer);
                int clipboard_len = strlen(clipboard_text);
                int available_space = buffer_size - current_len - 1;

                if (available_space > 0) {

                    int copy_len = (clipboard_len < available_space) ? clipboard_len : available_space;
                    memcpy(body_buffer + current_len, clipboard_text, copy_len);
                    body_buffer[current_len + copy_len] = '\0';
                }
            }
        }
    }

    DrawRectangleRec(text_area, theme->bg0);
    DrawRectangleLinesEx(text_area, 1, text_active ? theme->blue : theme->gray);

    if (strlen(body_buffer) > 0) {
        Vector2 text_pos = {text_area.x + 10, text_area.y + 10};

        if (*body_type == BODY_TYPE_JSON) {

            char* text = body_buffer;
            Vector2 current_pos = text_pos;
            float line_height = FONT_SIZE_NORMAL + 4;

            for (int i = 0; text[i] != '\0'; i++) {
                char current_char = text[i];
                Color char_color = theme->fg0;

                if (current_char == '"') {
                    char_color = theme->green;
                } else if (current_char >= '0' && current_char <= '9') {
                    char_color = theme->purple;
                } else if (current_char == '{' || current_char == '}' || 
                          current_char == '[' || current_char == ']' ||
                          current_char == ',' || current_char == ':') {
                    char_color = theme->yellow;
                }

                if (current_char == '\n') {
                    current_pos.y += line_height;
                    current_pos.x = text_pos.x;
                    continue;
                }

                char single_char[2] = {current_char, '\0'};
                DrawTextEx(GetCodeFont(), single_char, current_pos, FONT_SIZE_NORMAL, 1, char_color);
                current_pos.x += MeasureTextEx(GetCodeFont(), single_char, FONT_SIZE_NORMAL, 1).x;
            }

            if (text_active && cursor_blink_timer < 0.5f) {
                DrawRectangle(current_pos.x, current_pos.y, 2, FONT_SIZE_NORMAL, theme->fg0);
            }
        } else {

            char* text = body_buffer;
            Vector2 current_pos = text_pos;
            float line_height = FONT_SIZE_NORMAL + 4;

            for (int i = 0; text[i] != '\0'; i++) {
                char current_char = text[i];

                if (current_char == '\n') {
                    current_pos.y += line_height;
                    current_pos.x = text_pos.x;
                    continue;
                }

                char single_char[2] = {current_char, '\0'};
                DrawTextEx(GetCodeFont(), single_char, current_pos, FONT_SIZE_NORMAL, 1, theme->fg0);
                current_pos.x += MeasureTextEx(GetCodeFont(), single_char, FONT_SIZE_NORMAL, 1).x;
            }

            if (text_active && cursor_blink_timer < 0.5f) {
                DrawRectangle(current_pos.x, current_pos.y, 2, FONT_SIZE_NORMAL, theme->fg0);
            }
        }
    } else {

        Vector2 placeholder_pos = {text_area.x + 10, text_area.y + 10};
        const char* placeholder_text = *body_type == BODY_TYPE_JSON ? "Enter JSON body here..." : "Enter plain text body here...";
        DrawTextEx(GetCodeFont(), placeholder_text, placeholder_pos, FONT_SIZE_NORMAL, 1, Fade(theme->gray, 0.7f));

        if (text_active && cursor_blink_timer < 0.5f) {
            DrawRectangle(placeholder_pos.x, placeholder_pos.y, 2, FONT_SIZE_NORMAL, theme->fg0);
        }
    }
}