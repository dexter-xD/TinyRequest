#include "ui_components.h"


static bool method_dropdown_open = false;
static int method_hover_index = -1;

static const char* method_strings[] = {
    "GET",
    "POST",
    "PUT",
    "DELETE"
};

bool IsMethodDropdownOpen(void) {
    return method_dropdown_open;
}

void RenderMethodDropdownOverlay(Rectangle bounds, int* selected_method, GruvboxTheme* theme) {
    if (!selected_method || !theme || !method_dropdown_open) return;
    Rectangle button_bounds = bounds;
    button_bounds.height = 30;
    method_hover_index = -1;
    for (int i = 0; i < 4; i++) {
        Rectangle option_bounds = {
            bounds.x,
            bounds.y + button_bounds.height + (i * 30),
            bounds.width,
            30
        };
        bool option_hovered = CheckCollisionPointRec(GetMousePosition(), option_bounds);
        bool option_clicked = option_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (option_hovered) {
            method_hover_index = i;
        }

        if (option_clicked) {
            *selected_method = i;
            method_dropdown_open = false;
        }

        Color option_bg = theme->bg1;
        if (option_hovered) {
            option_bg = theme->bg2;
        } else if (i == *selected_method) {
            option_bg = BlendColors(theme->bg1, theme->orange, 0.2f);
        }
        DrawRectangleRec(option_bounds, option_bg);
        DrawRectangleLinesEx(option_bounds, 1, theme->gray);

        Vector2 option_text_size = MeasureTextEx(GetCodeFont(), method_strings[i], FONT_SIZE_NORMAL, 1);
        Vector2 option_text_pos = {
            option_bounds.x + PADDING_MEDIUM,
            option_bounds.y + (option_bounds.height - option_text_size.y) / 2
        };
        Color text_color = (i == *selected_method) ? theme->orange : theme->fg0;
        if (option_hovered) {
            text_color = theme->fg0;
        }
        DrawTextEx(GetCodeFont(), method_strings[i], option_text_pos, FONT_SIZE_NORMAL, 1, text_color);
    }
}

void RenderMethodSelector(Rectangle bounds, int* selected_method, GruvboxTheme* theme) {
    if (!selected_method || !theme) return;

    if (*selected_method < 0 || *selected_method >= 4) {
        *selected_method = HTTP_GET;
    }

    Rectangle button_bounds = bounds;
    button_bounds.height = 30;

    bool button_hovered = CheckCollisionPointRec(GetMousePosition(), button_bounds);
    bool button_pressed = button_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (button_pressed) {
        method_dropdown_open = !method_dropdown_open;
    }

    if (method_dropdown_open && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !button_hovered) {
        bool clicked_in_dropdown = false;
        if (method_dropdown_open) {
            Rectangle dropdown_bounds = {
                bounds.x,
                bounds.y + button_bounds.height,
                bounds.width,
                30 * 4
            };
            clicked_in_dropdown = CheckCollisionPointRec(GetMousePosition(), dropdown_bounds);
        }
        if (!clicked_in_dropdown) {
            method_dropdown_open = false;
        }
    }

    Color button_bg = button_hovered ? theme->bg2 : theme->bg1;
    Color button_border = button_hovered ? theme->orange : theme->gray;

    DrawRectangleRec(button_bounds, button_bg);
    DrawRectangleLinesEx(button_bounds, 1, button_border);

    const char* selected_text = method_strings[*selected_method];
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
    if (method_dropdown_open) {

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