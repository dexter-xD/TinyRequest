#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "raylib.h"
#include "../include/ui_components.h"
#include "../include/theme.h"
#include "../include/request_state.h"

int main(void) {

    InitWindow(800, 600, "Headers Editor Test");
    SetTargetFPS(60);

    GruvboxTheme theme = InitGruvboxTheme();

    char* headers[MAX_HEADERS] = {NULL};
    int header_count = 0;

    headers[0] = strdup("Content-Type: application/json");
    headers[1] = strdup("Authorization: Bearer token123");
    header_count = 2;

    printf("Headers Editor Test Started\n");
    printf("Initial headers:\n");
    for (int i = 0; i < header_count; i++) {
        printf("  %d: %s\n", i, headers[i]);
    }

    Rectangle editor_bounds = {50, 100, 700, 400};

    while (!WindowShouldClose()) {
        BeginDrawing();

        ApplyThemeToWindow(&theme);

        DrawText("Headers Editor Test", 50, 50, 20, theme.fg0);
        DrawText("Click to add/edit headers, press ESC to exit", 50, 70, 16, theme.fg1);

        RenderHeadersEditor(editor_bounds, headers, &header_count, &theme);

        char count_text[64];
        snprintf(count_text, sizeof(count_text), "Header count: %d", header_count);
        DrawText(count_text, 50, 520, 16, theme.fg0);

        int y_offset = 540;
        for (int i = 0; i < header_count && i < 3; i++) {
            if (headers[i]) {
                char display_text[128];
                snprintf(display_text, sizeof(display_text), "%d: %.80s", i, headers[i]);
                DrawText(display_text, 50, y_offset + (i * 20), 12, theme.fg1);
            }
        }

        EndDrawing();
    }

    for (int i = 0; i < MAX_HEADERS; i++) {
        if (headers[i]) {
            free(headers[i]);
        }
    }

    CloseWindow();

    printf("Headers Editor Test Completed\n");
    printf("Final header count: %d\n", header_count);

    return 0;
}