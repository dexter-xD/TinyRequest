#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "theme.h"
#include "ui_components.h"
#include "request_state.h"
#include "http_client.h"
#include "error_handler.h"
#include <string.h>
#include "ui_style.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define WINDOW_TITLE "TinyRequest"
#define TARGET_FPS 60

typedef enum {
    TAB_PARAMS = 0,
    TAB_BODY,
    TAB_AUTH,
    TAB_HEADERS,
    TAB_COUNT
} RequestTab;

typedef struct {
    GruvboxTheme theme;
    bool should_close;
    int selected_method;
    char url_buffer[MAX_URL_LENGTH];
    char body_buffer[MAX_BODY_LENGTH];
    char* headers[MAX_HEADERS];
    int header_count;
    HttpResponse* last_response;
    AppErrorHandler* error_handler;
    AsyncHttpHandle* current_request;
    bool request_in_progress;
    Texture2D logo_texture;
    RequestTab active_tab;
    BodyType body_type;
} AppState;

void InitializeApplication(AppState* app);
void UpdateApplication(AppState* app);
void RenderApplication(AppState* app);
void CleanupApplication(AppState* app);

int main(void) {

    AppState app = {0};

    InitializeApplication(&app);

    while (!app.should_close && !WindowShouldClose()) {
        UpdateApplication(&app);
        RenderApplication(&app);
    }

    CleanupApplication(&app);
    return 0;
}

void InitializeApplication(AppState* app) {

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);

    Image icon = LoadImage("assets/icon.bmp");
    if (icon.data != NULL && icon.width > 0 && icon.height > 0) {
        printf("Icon loaded: %dx%d, mipmaps=%d, format=%d\n", icon.width, icon.height, icon.mipmaps, icon.format);
        SetWindowIcon(icon);
        printf("SetWindowIcon called successfully.\n");
    } else {
        printf("Failed to load icon: assets/icon.bmp\n");
    }
    UnloadImage(icon);

    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetTargetFPS(TARGET_FPS);

    app->theme = InitGruvboxTheme();
    app->should_close = false;

    InitThemeFonts();

    app->selected_method = HTTP_GET;
    memset(app->url_buffer, 0, sizeof(app->url_buffer));
    memset(app->body_buffer, 0, sizeof(app->body_buffer));
    app->header_count = 0;
    for (int i = 0; i < MAX_HEADERS; i++) {
        app->headers[i] = NULL;
    }
    app->last_response = NULL;

    app->error_handler = CreateErrorHandler();
    if (!app->error_handler) {
        printf("Failed to initialize error handler\n");
        app->should_close = true;
        return;
    }

    if (!InitHttpClient()) {
        ReportGeneralError(app->error_handler, ERROR_LEVEL_ERROR, "Failed to initialize HTTP client");
        printf("Failed to initialize HTTP client\n");
    }

    app->current_request = NULL;
    app->request_in_progress = false;
    app->active_tab = TAB_BODY; 
    app->body_type = BODY_TYPE_JSON; 

    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetExitKey(KEY_NULL);

    app->logo_texture = LoadTexture("assets/logo.png");

    printf("TinyRequest initialized successfully\n");
    printf("Window size: %dx%d\n", WINDOW_WIDTH, WINDOW_HEIGHT);
    printf("Gruvbox theme loaded\n");
}

void UpdateApplication(AppState* app) {

    UpdateErrorHandler(app->error_handler);

    if (app->current_request) {
        AsyncRequestState state = CheckAsyncRequest(app->current_request);

        switch (state) {
            case ASYNC_REQUEST_COMPLETED: {
                HttpResponse* response = GetAsyncResponse(app->current_request);
                if (response) {

                    if (app->last_response) {
                        FreeHttpResponse(app->last_response);
                    }
                    app->last_response = response;

                    char success_msg[256];
                    snprintf(success_msg, sizeof(success_msg),
                            "Request completed: %d (%.2fms)",
                            response->status_code, response->response_time);
                    ReportGeneralError(app->error_handler, ERROR_LEVEL_SUCCESS, success_msg);
                } else {
                    ReportGeneralError(app->error_handler, ERROR_LEVEL_ERROR, "Request completed but no response received");
                }

                FreeAsyncHandle(app->current_request);
                app->current_request = NULL;
                app->request_in_progress = false;
                break;
            }

            case ASYNC_REQUEST_ERROR: {
                HttpError http_error = GetLastHttpError();
                ReportNetworkError(app->error_handler, &http_error);

                FreeAsyncHandle(app->current_request);
                app->current_request = NULL;
                app->request_in_progress = false;
                break;
            }

            case ASYNC_REQUEST_TIMEOUT: {
                ReportGeneralError(app->error_handler, ERROR_LEVEL_ERROR, "Request timed out after 30 seconds");

                FreeAsyncHandle(app->current_request);
                app->current_request = NULL;
                app->request_in_progress = false;
                break;
            }

            case ASYNC_REQUEST_CANCELLED: {
                ReportGeneralError(app->error_handler, ERROR_LEVEL_INFO, "Request cancelled by user");

                FreeAsyncHandle(app->current_request);
                app->current_request = NULL;
                app->request_in_progress = false;
                break;
            }

            case ASYNC_REQUEST_PENDING:

                break;
        }
    }

    static double last_send_click_time = 0.0;

    const int margin = 20;
    const int card_padding = 18;
    const int left_panel_width = GetScreenWidth() / 2 - margin - 10;
    const int titlebar_h = 44;
    const int component_spacing = 15;
    int current_y = titlebar_h + component_spacing;
    Rectangle request_card = {margin, current_y, left_panel_width, GetScreenHeight() - current_y - margin};
    int req_inner_x = request_card.x + card_padding;
    float button_w = 110;
    float button_h = 38;

    Rectangle send_button_bounds = {req_inner_x, request_card.y + request_card.height - button_h - card_padding, button_w, button_h};
    bool send_button_hovered = CheckCollisionPointRec(GetMousePosition(), send_button_bounds);
    bool send_button_clicked = send_button_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    double current_time = GetTime();
    if (send_button_clicked && (current_time - last_send_click_time) > 1.0) {  
        last_send_click_time = current_time;
        printf("DEBUG: Send button clicked\n");

        printf("DEBUG: Forcing complete state cleanup\n");

        if (app->current_request) {
            printf("DEBUG: Force cancelling existing request\n");
            CancelAsyncRequest(app->current_request);
            FreeAsyncHandle(app->current_request);
            app->current_request = NULL;
        }

        if (app->last_response) {
            printf("DEBUG: Force cleanup of previous response\n");
            FreeHttpResponse(app->last_response);
            app->last_response = NULL;
        }

        app->request_in_progress = false;

        printf("DEBUG: State cleanup complete, proceeding with request\n");

        if (!app->request_in_progress) {
            printf("DEBUG: Not in progress, validating inputs\n");

            ValidateAndReportUrl(app->error_handler, app->url_buffer);
            ValidateAndReportJson(app->error_handler, app->body_buffer);
            ValidateAndReportHeaders(app->error_handler, app->headers, app->header_count);

            printf("DEBUG: Validation complete, checking for errors\n");

            if (!HasActiveErrors(app->error_handler)) {
                printf("DEBUG: No validation errors, creating HTTP request\n");

                HttpRequest* request = CreateHttpRequest();
                if (request) {
                    printf("DEBUG: HTTP request created successfully\n");
                    SetHttpRequestUrl(request, app->url_buffer);
                    printf("DEBUG: URL set to: %s\n", app->url_buffer);
                    SetHttpRequestMethod(request, GetHttpMethodString(app->selected_method));
                    printf("DEBUG: Method set to: %s\n", GetHttpMethodString(app->selected_method));

                    if (app->selected_method == HTTP_POST || app->selected_method == HTTP_PUT) {
                        if (strlen(app->body_buffer) > 0) {

                            printf("DEBUG: Original body buffer: [%s]\n", app->body_buffer);
                            char* processed_body = ProcessBodyEscapeSequences(app->body_buffer);
                            if (processed_body) {
                                printf("DEBUG: Processed body: [%s]\n", processed_body);
                                SetHttpRequestBody(request, processed_body);
                                printf("DEBUG: Body set (original length: %zu, processed length: %zu)\n", 
                                       strlen(app->body_buffer), strlen(processed_body));
                                free(processed_body);

                            } else {
                                ReportGeneralError(app->error_handler, ERROR_LEVEL_WARNING,
                                                    "Failed to process body escape sequences.  Using original body.");
                                
                                SetHttpRequestBody(request, app->body_buffer);
                                printf("DEBUG: Body set without processing (length: %zu)\n", strlen(app->body_buffer));
                            }
                        } else {

                            SetHttpRequestBody(request, "{}");
                            printf("DEBUG: Added default empty JSON body for POST request\n");
                        }

                        if (app->body_type == BODY_TYPE_JSON) {
                            AddHttpRequestHeader(request, "Content-Type", "application/json");
                            printf("DEBUG: Added Content-Type: application/json header\n");
                        } else {
                            AddHttpRequestHeader(request, "Content-Type", "text/plain");
                            printf("DEBUG: Added Content-Type: text/plain header\n");
                        }
                    } else if (strlen(app->body_buffer) > 0) {

                        printf("DEBUG: Original body buffer: [%s]\n", app->body_buffer);
                        char* processed_body = ProcessBodyEscapeSequences(app->body_buffer);
                        if (processed_body) {


                            printf("DEBUG: Processed body: [%s]\n", processed_body);
                            SetHttpRequestBody(request, processed_body);
                            printf("DEBUG: Body set (original length: %zu, processed length: %zu)\n", 
                                   strlen(app->body_buffer), strlen(processed_body));
                            free(processed_body);
                        } else {
                            SetHttpRequestBody(request, app->body_buffer);
                            ReportGeneralError(app->error_handler, ERROR_LEVEL_WARNING,
                                                    "Failed to process body escape sequences.  Using original body.");

                            printf("DEBUG: Body set without processing (length: %zu)\n", strlen(app->body_buffer));
                        }
                    }

                    printf("DEBUG: Adding %d headers\n", app->header_count);
                    for (int i = 0; i < app->header_count; i++) {
                        if (app->headers[i] && strlen(app->headers[i]) > 0) {
                            char* colon_pos = strchr(app->headers[i], ':');
                            if (colon_pos) {

                                size_t key_len = colon_pos - app->headers[i];
                                char* key = malloc(key_len + 1);
                                if (key) {
                                    strncpy(key, app->headers[i], key_len);
                                    key[key_len] = '\0';

                                    char* value = colon_pos + 1;
                                    while (*value == ' ' || *value == '\t') value++;

                                    char* key_copy = strdup(key);
                                    char* value_copy = strdup(value);

                                    AddHttpRequestHeader(request, key_copy, value_copy);
                                    printf("DEBUG: Added header: %s: %s\n", key, value);
                                    free(key);
                                }

                            }
                        }
                    }

                    printf("DEBUG: Sending async HTTP request\n");
                    app->current_request = SendHttpRequestAsync(request);
                    if (app->current_request) {
                        printf("DEBUG: Async request created successfully\n");
                        app->request_in_progress = true;
                        ReportGeneralError(app->error_handler, ERROR_LEVEL_INFO, "Sending request...");
                    } else {
                        printf("DEBUG: Failed to create async request\n");
                        HttpError http_error = GetLastHttpError();
                        printf("DEBUG: HTTP error code: %d, message: %s\n", http_error.code, http_error.message);
                        ReportNetworkError(app->error_handler, &http_error);
                    }

                } else {
                    printf("DEBUG: Failed to create HTTP request\n");
                    ReportGeneralError(app->error_handler, ERROR_LEVEL_ERROR, "Failed to create HTTP request");
                }
            }
        } else {

            if (app->current_request) {
                CancelAsyncRequest(app->current_request);
            }
        }
    }

    if (IsKeyPressed(KEY_F4) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))) {
        app->should_close = true;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        app->should_close = true;
    }

    if (IsWindowResized()) {
        int width = GetScreenWidth();
        int height = GetScreenHeight();
        printf("Window resized to: %dx%d\n", width, height);
    }
}

void RenderApplication(AppState* app) {
    BeginDrawing();

    ApplyThemeToWindow(&app->theme);
    int window_width = GetScreenWidth();
    int window_height = GetScreenHeight();

    const int margin = 20;
    const int component_spacing = 15;
    const int card_padding = 18;
    const int left_panel_width = window_width / 2 - margin - 10;
    const int right_panel_width = window_width / 2 - margin - 10;

    int titlebar_h = 44;
    DrawRectangle(0, 0, window_width, titlebar_h, app->theme.bg2);
    DrawLine(0, titlebar_h, window_width, titlebar_h, app->theme.gray);

    int logo_h = 32;
    int logo_w = (app->logo_texture.height > 0) ? (app->logo_texture.width * logo_h / app->logo_texture.height) : 0;
    if (app->logo_texture.id > 0) {
        DrawTexturePro(app->logo_texture,
            (Rectangle){0, 0, app->logo_texture.width, app->logo_texture.height},
            (Rectangle){margin, (titlebar_h - logo_h) / 2, logo_w, logo_h},
            (Vector2){0, 0}, 0, WHITE);
    }

    const char* title_text = "TinyRequest";
    int title_font_size = FONT_SIZE_LARGE + 4;
    Vector2 title_size = MeasureTextEx(GetTitleFont(), title_text, title_font_size, 1.0f);
    Vector2 title_pos = {margin + logo_w + 12, (titlebar_h - title_size.y) / 2};

    Vector2 shadow_pos = {title_pos.x + 1, title_pos.y + 1};
    DrawTextEx(GetTitleFont(), title_text, shadow_pos, title_font_size, 1.0f, Fade(app->theme.bg0, 0.6f));
    DrawTextEx(GetTitleFont(), title_text, title_pos, title_font_size, 1.0f, app->theme.fg0);

    int btn_w = 38, btn_h = 32, btn_gap = 8;
    int btn_y = (titlebar_h - btn_h) / 2;
    int btn_x_close = window_width - margin - btn_w;
    int btn_x_max = btn_x_close - btn_w - btn_gap;
    int btn_x_min = btn_x_max - btn_w - btn_gap;
    Rectangle btn_min = {btn_x_min, btn_y, btn_w, btn_h};
    Rectangle btn_max = {btn_x_max, btn_y, btn_w, btn_h};
    Rectangle btn_close = {btn_x_close, btn_y, btn_w, btn_h};

    Vector2 mouse = GetMousePosition();
    bool min_hover = CheckCollisionPointRec(mouse, btn_min);
    bool max_hover = CheckCollisionPointRec(mouse, btn_max);
    bool close_hover = CheckCollisionPointRec(mouse, btn_close);
    DrawRectangleRec(btn_min, min_hover ? app->theme.bg2 : app->theme.bg1);
    DrawRectangleRec(btn_max, max_hover ? app->theme.bg2 : app->theme.bg1);
    DrawRectangleRec(btn_close, close_hover ? app->theme.red : app->theme.bg1);
    DrawRectangleLinesEx(btn_min, 2, app->theme.gray);
    DrawRectangleLinesEx(btn_max, 2, app->theme.gray);
    DrawRectangleLinesEx(btn_close, 2, app->theme.gray);

    float min_line_width = btn_w * 0.5f;
    float min_line_thickness = 2.0f;
    Vector2 min_center = {btn_min.x + btn_w/2, btn_min.y + btn_h/2};
    DrawRectangle(min_center.x - min_line_width/2, min_center.y - min_line_thickness/2, 
                  min_line_width, min_line_thickness, min_hover ? app->theme.fg0 : app->theme.fg1);

    float max_size = btn_w * 0.45f;
    Vector2 max_center = {btn_max.x + btn_w/2, btn_max.y + btn_h/2};
    if (IsWindowFullscreen()) {

        float offset = 3;
        DrawRectangleLines(max_center.x - max_size/2 + offset, max_center.y - max_size/2 - offset, 
                          max_size - offset, max_size - offset, max_hover ? app->theme.fg0 : app->theme.fg1);
        DrawRectangleLines(max_center.x - max_size/2, max_center.y - max_size/2 + offset, 
                          max_size - offset, max_size - offset, max_hover ? app->theme.fg0 : app->theme.fg1);
    } else {

        DrawRectangleLines(max_center.x - max_size/2, max_center.y - max_size/2, 
                          max_size, max_size, max_hover ? app->theme.fg0 : app->theme.fg1);
    }

    float close_size = btn_w * 0.4f;
    Vector2 close_center = {btn_close.x + btn_w/2, btn_close.y + btn_h/2};
    float close_thickness = 2.0f;
    Color close_color = close_hover ? WHITE : app->theme.fg1;

    Vector2 x1_start = {close_center.x - close_size/2, close_center.y - close_size/2};
    Vector2 x1_end = {close_center.x + close_size/2, close_center.y + close_size/2};
    Vector2 x2_start = {close_center.x + close_size/2, close_center.y - close_size/2};
    Vector2 x2_end = {close_center.x - close_size/2, close_center.y + close_size/2};

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (abs(i) + abs(j) <= 1) { 
                DrawLineV((Vector2){x1_start.x + i, x1_start.y + j}, 
                         (Vector2){x1_end.x + i, x1_end.y + j}, close_color);
                DrawLineV((Vector2){x2_start.x + i, x2_start.y + j}, 
                         (Vector2){x2_end.x + i, x2_end.y + j}, close_color);
            }
        }
    }

    static bool is_maximized = false;
    static Vector2 prev_win_pos = {0};
    static int prev_win_w = 0, prev_win_h = 0;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (min_hover) MinimizeWindow();
        else if (max_hover) {
            if (!is_maximized) {
                prev_win_pos = GetWindowPosition();
                prev_win_w = GetScreenWidth();
                prev_win_h = GetScreenHeight();
                MaximizeWindow();
                is_maximized = true;
            } else {
                RestoreWindow();
                SetWindowPosition((int)prev_win_pos.x, (int)prev_win_pos.y);
                SetWindowSize(prev_win_w, prev_win_h);
                is_maximized = false;
            }
        }
        else if (close_hover) CloseWindow();
    }

    static bool dragging = false;
    static Vector2 drag_offset = {0};
    Rectangle drag_bar = {0, 0, window_width - (btn_w + btn_gap) * 3 - margin, titlebar_h};
    if (!min_hover && !max_hover && !close_hover && CheckCollisionPointRec(mouse, drag_bar)) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            dragging = true;
            drag_offset = mouse;
        }
    }
    if (dragging) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 win_pos = GetWindowPosition();
            SetWindowPosition((int)(win_pos.x + (mouse.x - drag_offset.x)), (int)(win_pos.y + (mouse.y - drag_offset.y)));
        } else {
            dragging = false;
        }
    }

    static double last_click_time = 0;
    if (CheckCollisionPointRec(mouse, drag_bar) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        double now = GetTime();
        if (now - last_click_time < 0.35) {

            if (!is_maximized) {
                prev_win_pos = GetWindowPosition();
                prev_win_w = GetScreenWidth();
                prev_win_h = GetScreenHeight();
                MaximizeWindow();
                is_maximized = true;
            } else {
                RestoreWindow();
                SetWindowPosition((int)prev_win_pos.x, (int)prev_win_pos.y);
                SetWindowSize(prev_win_w, prev_win_h);
                is_maximized = false;
            }
        }
        last_click_time = now;
    }

    int current_y = titlebar_h + component_spacing;

    Rectangle request_card = {margin, current_y, left_panel_width, window_height - current_y - margin};
    DrawPanelCard(request_card, &app->theme);
    int req_inner_x = request_card.x + card_padding;
    int req_inner_y = request_card.y + card_padding;
    int req_inner_w = request_card.width - 2 * card_padding;

    Rectangle method_bounds = {req_inner_x, req_inner_y, 120, 35};
    RenderMethodSelector(method_bounds, &app->selected_method, &app->theme);

    Rectangle url_bounds = {req_inner_x + 130, req_inner_y, req_inner_w - 130, 35};
    RenderUrlInput(url_bounds, app->url_buffer, sizeof(app->url_buffer), &app->theme);
    req_inner_y += 45;

    const char* tab_names[] = {"Params", "Body", "Auth", "Headers"};
    const int tab_count = TAB_COUNT;
    const float tab_height = 35;
    const float tab_width = req_inner_w / tab_count;

    Rectangle tab_bar = {req_inner_x, req_inner_y, req_inner_w, tab_height};

    Vector2 mouse_pos = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_pos, tab_bar)) {
        int clicked_tab = (int)((mouse_pos.x - tab_bar.x) / tab_width);
        if (clicked_tab >= 0 && clicked_tab < tab_count) {
            app->active_tab = (RequestTab)clicked_tab;
        }
    }

    for (int i = 0; i < tab_count; i++) {
        Rectangle tab_rect = {req_inner_x + i * tab_width, req_inner_y, tab_width, tab_height};
        bool is_active = (app->active_tab == i);
        bool is_hovered = CheckCollisionPointRec(mouse_pos, tab_rect);

        Color tab_bg = is_active ? app->theme.bg2 : (is_hovered ? app->theme.bg1 : app->theme.bg0);
        Color tab_border = is_active ? app->theme.blue : app->theme.gray;
        Color text_color = is_active ? app->theme.fg0 : app->theme.fg1;

        DrawRectangleRec(tab_rect, tab_bg);
        if (is_active) {
            DrawRectangle(tab_rect.x, tab_rect.y + tab_rect.height - 2, tab_rect.width, 2, app->theme.blue);
        }
        DrawRectangleLinesEx(tab_rect, 1, tab_border);

        const char* display_text = tab_names[i];
        char tab_text_with_count[64];
        if (i == TAB_HEADERS && app->header_count > 0) {
            snprintf(tab_text_with_count, sizeof(tab_text_with_count), "%s (%d)", tab_names[i], app->header_count);
            display_text = tab_text_with_count;
        } else if (i == TAB_BODY && strlen(app->body_buffer) > 0) {
            snprintf(tab_text_with_count, sizeof(tab_text_with_count), "%s ●", tab_names[i]);
            display_text = tab_text_with_count;
        } else if (i == TAB_AUTH) {
            snprintf(tab_text_with_count, sizeof(tab_text_with_count), "%s ●", tab_names[i]);
            display_text = tab_text_with_count;
        }

        Vector2 text_size = MeasureTextEx(GetBoldFont(), display_text, FONT_SIZE_SMALL, 1);
        Vector2 text_pos = {
            tab_rect.x + (tab_rect.width - text_size.x) / 2,
            tab_rect.y + (tab_rect.height - text_size.y) / 2
        };
        DrawTextEx(GetBoldFont(), display_text, text_pos, FONT_SIZE_SMALL, 1, text_color);
    }

    req_inner_y += tab_height + 10;

    Rectangle content_bounds = {req_inner_x, req_inner_y, req_inner_w, request_card.y + request_card.height - req_inner_y - 60};

    switch (app->active_tab) {
        case TAB_PARAMS:

            DrawRectangleRec(content_bounds, app->theme.bg1);
            DrawRectangleLinesEx(content_bounds, 1, app->theme.gray);
            Vector2 params_text_pos = {content_bounds.x + 10, content_bounds.y + 10};
            DrawTextEx(GetCodeFont(), "URL Parameters (Coming Soon)", params_text_pos, FONT_SIZE_NORMAL, 1, app->theme.gray);
            break;

        case TAB_BODY:
            RenderBodyEditor(content_bounds, app->body_buffer, sizeof(app->body_buffer), &app->theme, &app->body_type);
            break;

        case TAB_AUTH:

            DrawRectangleRec(content_bounds, app->theme.bg1);
            DrawRectangleLinesEx(content_bounds, 1, app->theme.gray);
            Vector2 auth_text_pos = {content_bounds.x + 10, content_bounds.y + 10};
            DrawTextEx(GetCodeFont(), "Authentication (Coming Soon)", auth_text_pos, FONT_SIZE_NORMAL, 1, app->theme.gray);
            break;

        case TAB_HEADERS:
            RenderHeadersEditor(content_bounds, app->headers, &app->header_count, &app->theme);
            break;
    }

    float button_w = 110;
    float button_h = 38;
    float status_gap = 14;
    float status_w = req_inner_w - button_w - status_gap;
    Rectangle send_button_bounds = {req_inner_x, request_card.y + request_card.height - button_h - card_padding, button_w, button_h};
    Rectangle status_bounds = {send_button_bounds.x + send_button_bounds.width + status_gap, send_button_bounds.y, status_w, button_h};
    bool send_hovered = CheckCollisionPointRec(GetMousePosition(), send_button_bounds);
    const char* button_text;
    Color button_bg, button_border;
    if (app->request_in_progress) {
        button_text = "Cancel";
        button_bg = send_hovered ? app->theme.red : BlendColors(app->theme.bg1, app->theme.red, 0.3f);
        button_border = app->theme.red;
        DrawButtonWithIcon(send_button_bounds, button_text, send_hovered, &app->theme, NULL);
    } else {
        button_text = "Send";
        button_bg = send_hovered ? app->theme.green : BlendColors(app->theme.bg1, app->theme.green, 0.3f);
        button_border = app->theme.green;
        extern void DrawPaperPlaneIcon(Vector2 center, float size, Color color);
        DrawButtonWithIcon(send_button_bounds, button_text, send_hovered, &app->theme, DrawPaperPlaneIcon);
    }
    if (app->request_in_progress) {
        Vector2 spinner_center = {
            send_button_bounds.x + send_button_bounds.width + 15,
            send_button_bounds.y + send_button_bounds.height / 2
        };
        RenderLoadingSpinner(spinner_center, 8, &app->theme);
    }

    RenderErrorHandler(app->error_handler, status_bounds, &app->theme);

    int right_panel_x = window_width / 2 + 10;
    Rectangle response_card = {right_panel_x, current_y, right_panel_width, window_height - current_y - margin};
    DrawPanelCard(response_card, &app->theme);
    int resp_inner_x = response_card.x + card_padding;
    int resp_inner_y = response_card.y + card_padding;

    DrawTextEx(GetUIFont(), "Response", (Vector2){resp_inner_x, resp_inner_y}, FONT_SIZE_LARGE, 1, app->theme.orange);
    resp_inner_y += FONT_SIZE_LARGE + 8;
    Rectangle response_bounds = {resp_inner_x, resp_inner_y, response_card.width - 2 * card_padding, response_card.height - resp_inner_y + response_card.y - 20};
    RenderResponseViewer(response_bounds, app->last_response, &app->theme);

    RenderErrorTooltips(app->error_handler, &app->theme);

    if (IsMethodDropdownOpen()) {
        RenderMethodDropdownOverlay(method_bounds, &app->selected_method, &app->theme);
    }

    if (IsBodyTypeDropdownOpen()) {

        Rectangle body_type_bounds = {req_inner_x + 10, content_bounds.y + 10, 100, 30};
        RenderBodyTypeDropdownOverlay(body_type_bounds, &app->body_type, &app->theme);
    }

    EndDrawing();
}

void CleanupApplication(AppState* app) {

    if (app->current_request) {
        CancelAsyncRequest(app->current_request);
        FreeAsyncHandle(app->current_request);
        app->current_request = NULL;
    }

    for (int i = 0; i < app->header_count; i++) {
        if (app->headers[i]) {
            free(app->headers[i]);
            app->headers[i] = NULL;
        }
    }

    if (app->last_response) {
        FreeHttpResponse(app->last_response);
        app->last_response = NULL;
    }

    if (app->error_handler) {
        FreeErrorHandler(app->error_handler);
        app->error_handler = NULL;
    }

    CleanupHttpClient();

    UnloadThemeFonts();

    if (app->logo_texture.id > 0) {
        UnloadTexture(app->logo_texture);
    }

    CloseWindow();

    printf("TinyRequest cleanup completed\n");
}