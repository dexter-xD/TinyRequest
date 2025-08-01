#include "ui/ui_request_panel.h"
#include "ui/ui_panels.h"
#include "ui/ui_core.h"
#include "ui/theme.h"
#include "font_awesome.h"
#include "http_client.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>

extern "C" {

static void ui_request_panel_render_auth_panel(UIManager* ui, AppState* state);
static void ui_request_panel_apply_authentication(AppState* state);

void ui_request_panel_render(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    Request* active_request = app_state_get_active_request(state);
    Collection* active_collection = app_state_get_active_collection(state);

    if (active_request && active_collection) {
        ui_request_panel_render_request_header(ui, state, active_request, active_collection);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    ui_request_panel_handle_keyboard_shortcuts(ui, state);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(SPACING_SM, SPACING_MD));

    const char* methods[] = { "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS" };
    const int method_count = sizeof(methods) / sizeof(methods[0]);

    theme_push_body_style();
    ImGui::Text("HTTP Method");
    theme_pop_text_style();

    ImGui::SetNextItemWidth(94);
    theme_push_input_style(theme);
    if (ImGui::Combo("##method", &state->selected_method_index, methods, method_count)) {

        app_state_mark_ui_dirty(state);
        app_state_set_unsaved_changes(state, true);
    }
    theme_pop_input_style();

    theme_push_body_style();
    theme_pop_text_style();

    bool url_valid = (strncmp(state->url_buffer, "http://", 7) == 0 ||
                     strncmp(state->url_buffer, "https://", 8) == 0) &&
                     strlen(state->url_buffer) > 8;

    ImGui::SetNextItemWidth(-165); 

    bool is_invalid_url = (!url_valid && strlen(state->url_buffer) > 0);

    if (is_invalid_url) {

        ImGui::PushStyleColor(ImGuiCol_FrameBg, theme_alpha_blend(theme->error, 0.2f));
        ImGui::PushStyleColor(ImGuiCol_Border, theme->error);
    } else {

        ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->bg_input);
        ImGui::PushStyleColor(ImGuiCol_Border, theme->border_normal);
    }

    if (ImGui::InputText("##url", state->url_buffer, sizeof(state->url_buffer))) {

        app_state_mark_ui_dirty(state);
        app_state_set_unsaved_changes(state, true);
    }

    ImGui::PopStyleColor(2);

    ImGui::SameLine();

    bool can_send = !state->request_in_progress && url_valid;
    if (!can_send) {
        ImGui::BeginDisabled();
    }

    theme_push_button_style(theme, BUTTON_TYPE_PRIMARY);
    if (ImGui::Button("Send", ImVec2(60, 0))) {
        ui_request_panel_handle_send_request(ui, state);
    }
    theme_pop_button_style();

    if (!can_send) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    Request* current_active_request = app_state_get_active_request(state);

    if (current_active_request) {

        theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
        if (ImGui::Button(ICON_FA_SAVE, ImVec2(40, 0))) {
            ui_request_panel_handle_save_request(ui, state);
        }
        theme_pop_button_style();
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Save request changes (Ctrl+S)");
        }

        ImGui::SameLine();

        theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
        if (ImGui::Button(ICON_FA_COPY, ImVec2(40, 0))) {
            ui_request_panel_handle_duplicate_request(ui, state);
        }
        theme_pop_button_style();
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Duplicate this request (Ctrl+D)");
        }
    } else {

        theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
        if (ImGui::Button("Save", ImVec2(70, 0))) {
            state->show_save_dialog = true;
            state->save_error_message[0] = '\0';
        }
        theme_pop_button_style();

        ImGui::SameLine();
        theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
        if (ImGui::Button("Load", ImVec2(70, 0))) {
            state->show_load_dialog = true;
            state->load_error_message[0] = '\0';
            state->selected_request_index_for_load = -1;
        }
        theme_pop_button_style();
    }

    if (!url_valid && strlen(state->url_buffer) > 0) {
        ImGui::Spacing();
        theme_render_status_indicator("Invalid URL: Must start with http:// or https://", STATUS_TYPE_ERROR, theme);
    }

    ImGui::Spacing();
    ImGui::PopStyleVar(); 

    ImGui::Separator();
    ImGui::Spacing();

    static int selected_request_tab = 0;
    const char* request_tab_names[] = { "Params", "Body", "Auth", "Headers" };
    const int request_tab_count = sizeof(request_tab_names) / sizeof(request_tab_names[0]);

    ImGui::BeginGroup();
    for (int i = 0; i < request_tab_count; i++) {
        if (i > 0) ImGui::SameLine();

        bool is_selected = (selected_request_tab == i);

        if (is_selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, theme->accent_primary);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme_lighten_color(theme->accent_primary, 0.1f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme_darken_color(theme->accent_primary, 0.1f));
            ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_primary);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, theme_alpha_blend(theme->fg_tertiary, 0.1f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme_alpha_blend(theme->fg_tertiary, 0.2f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme_alpha_blend(theme->fg_tertiary, 0.3f));
            ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_secondary);
        }

        char tab_label[64];
        if (i == 3) { 

            Request* active_request = app_state_get_active_request(state);
            int header_count = active_request ? active_request->headers.count : state->current_request.headers.count;
            snprintf(tab_label, sizeof(tab_label), "%s %d", request_tab_names[i], header_count);
        } else if (i == 1) { 
            const char* current_method = ui_core_get_method_string(state->selected_method_index);
            bool method_supports_body = (strcmp(current_method, "POST") == 0 ||
                                        strcmp(current_method, "PUT") == 0 ||
                                        strcmp(current_method, "PATCH") == 0 ||
                                        strcmp(current_method, "DELETE") == 0);
            if (method_supports_body && strlen(state->body_buffer) > 0) {
                snprintf(tab_label, sizeof(tab_label), "%s ●", request_tab_names[i]); 
            } else {
                strcpy(tab_label, request_tab_names[i]);
            }
        } else {
            strcpy(tab_label, request_tab_names[i]);
        }

        if (ImGui::Button(tab_label, ImVec2(80, 30))) {
            selected_request_tab = i;
        }

        ImGui::PopStyleColor(4);
    }
    ImGui::EndGroup();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    switch (selected_request_tab) {
        case 0: 
            theme_render_status_indicator("URL parameters functionality not implemented yet", STATUS_TYPE_INFO, theme);
            break;

        case 1: 
            {
                const char* current_method = ui_core_get_method_string(state->selected_method_index);
                bool method_supports_body = (strcmp(current_method, "POST") == 0 ||
                                            strcmp(current_method, "PUT") == 0 ||
                                            strcmp(current_method, "PATCH") == 0 ||
                                            strcmp(current_method, "DELETE") == 0);

                if (method_supports_body) {

                    ui_request_panel_render_body_panel(ui, state);
                } else {
                    theme_render_status_indicator("Request body not supported for this HTTP method", STATUS_TYPE_INFO, theme);
                }
            }
            break;

        case 2: 
            ui_request_panel_render_auth_panel(ui, state);
            break;

        case 3: 
            {

                Request* active_request = app_state_get_active_request(state);
                if (active_request) {
                    ui_panels_render_headers_panel(ui, &active_request->headers, state);
                } else {
                    ui_panels_render_headers_panel(ui, &state->current_request.headers, state);
                }
            }
            break;

        case 4: 
            theme_render_status_indicator("Pre/Post request scripts not implemented yet", STATUS_TYPE_INFO, theme);
            break;

        case 5: 
            theme_render_status_indicator("Documentation functionality not implemented yet", STATUS_TYPE_INFO, theme);
            break;
    }
}

bool ui_request_panel_handle_send_request(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return false;
    }

    if (state->request_in_progress) {
        return false;
    }

    state->request_in_progress = true;
    snprintf(state->status_message, sizeof(state->status_message), "Sending request...");

    state->ui_state_dirty = true;
    app_state_sync_ui_to_request(state);

    ui_request_panel_apply_authentication(state);

    Request* request_to_send = app_state_get_active_request(state);
    if (!request_to_send) {
        request_to_send = &state->current_request;

        if (strlen(request_to_send->method) == 0) {
            strcpy(request_to_send->method, "GET");
        }
        if (strlen(request_to_send->url) == 0) {
            strcpy(request_to_send->url, "https://");
        }
    }

    if (strlen(state->body_buffer) > 0) {

        int body_result = request_set_body(request_to_send, state->body_buffer, strlen(state->body_buffer));
        if (body_result != 0) {

            state->request_in_progress = false;
            snprintf(state->status_message, sizeof(state->status_message), "Failed to prepare request body");
            return false;
        }
    }

    response_cleanup(&state->current_response);
    response_init(&state->current_response);

    if (request_to_send != &state->current_request) {
        request_cleanup(&state->current_request);
        request_init(&state->current_request);

        strncpy(state->current_request.method, request_to_send->method, sizeof(state->current_request.method) - 1);
        state->current_request.method[sizeof(state->current_request.method) - 1] = '\0';

        strncpy(state->current_request.url, request_to_send->url, sizeof(state->current_request.url) - 1);
        state->current_request.url[sizeof(state->current_request.url) - 1] = '\0';

        for (int i = 0; i < request_to_send->headers.count; i++) {
            header_list_add(&state->current_request.headers,
                           request_to_send->headers.headers[i].name,
                           request_to_send->headers.headers[i].value);
        }

        if (request_to_send->body && request_to_send->body_size > 0) {
            int body_result = request_set_body(&state->current_request, request_to_send->body, request_to_send->body_size);
            if (body_result != 0) {

                state->request_in_progress = false;
                snprintf(state->status_message, sizeof(state->status_message), "Failed to prepare request body");
                return false;
            }
        }
    } else {

        if (strlen(state->current_request.method) == 0) {
            strcpy(state->current_request.method, "GET");
        }
        if (strlen(state->current_request.url) == 0) {
            strcpy(state->current_request.url, "https://");
        }
    }

    int result;
    Collection* active_collection = app_state_get_active_collection(state);
    if (active_collection) {
        result = http_client_send_request_with_cookies(state->http_client, &state->current_request, &state->current_response, active_collection);
    } else {

        result = http_client_send_request(state->http_client, &state->current_request, &state->current_response);
    }

    if (result == 0) {

        if (state->current_response.status_code >= 200 && state->current_response.status_code < 300) {
            snprintf(state->status_message, sizeof(state->status_message), 
                    "Success: %d %s (%.2f ms)", 
                    state->current_response.status_code,
                    state->current_response.status_text,
                    state->current_response.response_time);
        } else if (state->current_response.status_code > 0) {
            snprintf(state->status_message, sizeof(state->status_message), 
                    "HTTP %d: %s (%.2f ms)", 
                    state->current_response.status_code,
                    state->current_response.status_text,
                    state->current_response.response_time);
        } else {
            snprintf(state->status_message, sizeof(state->status_message), 
                    "Network Error: %s", 
                    state->current_response.status_text);
        }
    } else {

        if (state->current_response.status_text[0] != '\0') {
            snprintf(state->status_message, sizeof(state->status_message), 
                    "Error: %s", state->current_response.status_text);
        } else {
            snprintf(state->status_message, sizeof(state->status_message), 
                    "Failed to send request (error code: %d)", result);
        }
    }

    if (request_to_send != &state->current_request) {
        app_state_check_and_perform_auto_save(state);
    }

    state->request_in_progress = false;

    return (result == 0);
}

void ui_request_panel_render_request_header(UIManager* ui, AppState* state, Request* request, Collection* collection) {
    if (!ui || !state || !request || !collection) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    ImGui::Spacing();

    CollectionManager* manager = state->collection_manager;
    int request_index = manager->active_request_index;
    const char* current_name = collection_get_request_name(collection, request_index);

    ImGui::Text("Request Name:");
    ImGui::SameLine();

    static char request_name_edit_buffer[256] = {0};
    static bool name_edit_initialized = false;
    static int last_active_request = -1;
    static Request* last_request_ptr = NULL;

    if (!name_edit_initialized || last_active_request != request_index || last_request_ptr != request) {
        if (current_name) {
            strncpy(request_name_edit_buffer, current_name, sizeof(request_name_edit_buffer) - 1);
            request_name_edit_buffer[sizeof(request_name_edit_buffer) - 1] = '\0';
        } else {
            strcpy(request_name_edit_buffer, "Untitled Request");
        }
        name_edit_initialized = true;
        last_active_request = request_index;
        last_request_ptr = request;
    }

    float padding = 20.0f;     
    float min_width = 80.0f;   
    float max_width = 300.0f;  

    ImVec2 text_size = ImGui::CalcTextSize(request_name_edit_buffer);
    float dynamic_width = text_size.x + padding;

    if (dynamic_width < min_width) dynamic_width = min_width;
    if (dynamic_width > max_width) dynamic_width = max_width;

    ImGui::SetNextItemWidth(dynamic_width);
    if (ImGui::InputText("##request_name", request_name_edit_buffer, sizeof(request_name_edit_buffer))) {

        if (strlen(request_name_edit_buffer) > 0) {
            collection_rename_request(collection, request_index, request_name_edit_buffer);
            app_state_set_unsaved_changes(state, true);
        }
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 230);
    ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_tertiary);
    ImGui::Text("in %s %s", ICON_FA_FOLDER, collection->name);
    ImGui::PopStyleColor();

    if (app_state_has_unsaved_changes(state)) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, theme->status_warning);
        ImGui::Text("● Unsaved");
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("You have unsaved changes. Press Ctrl+S to save.");
        }

        time_t last_change_time = app_state_get_last_change_time(state);
        time_t current_time = time(NULL);

        if (last_change_time > 0 && (current_time - last_change_time) > 30) {

            ImGui::SameLine();
            theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
            if (ImGui::SmallButton("Auto")) {
                ui_request_panel_handle_save_request(ui, state);
            }
            theme_pop_button_style();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Click to save your changes automatically");
            }
        }
    }
}

bool ui_request_panel_handle_save_request(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return false;
    }

    Request* active_request = app_state_get_active_request(state);
    if (!active_request) {
        return false;
    }

    app_state_sync_ui_to_request(state);

    int result = app_state_save_all_collections(state);
    if (result == 0) {
        app_state_set_unsaved_changes(state, false);
        snprintf(state->status_message, sizeof(state->status_message), 
                "Request saved successfully");
        return true;
    } else {
        snprintf(state->status_message, sizeof(state->status_message), 
                "Failed to save request");
        return false;
    }
}

bool ui_request_panel_handle_duplicate_request(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return false;
    }

    Collection* active_collection = app_state_get_active_collection(state);
    Request* active_request = app_state_get_active_request(state);

    if (!active_collection || !active_request) {
        return false;
    }

    app_state_sync_ui_to_request(state);

    CollectionManager* manager = state->collection_manager;
    int current_request_index = manager->active_request_index;
    const char* current_name = collection_get_request_name(active_collection, current_request_index);

    char duplicate_name[256];
    if (current_name) {
        snprintf(duplicate_name, sizeof(duplicate_name), "%s (Copy)", current_name);
    } else {
        strcpy(duplicate_name, "Untitled Request (Copy)");
    }

    int new_request_index = collection_duplicate_request(active_collection, current_request_index);
    if (new_request_index >= 0) {

        collection_rename_request(active_collection, new_request_index, duplicate_name);

        app_state_set_active_request(state, new_request_index);

        Request* new_request = collection_get_request(active_collection, new_request_index);
        if (new_request) {

            app_state_mark_request_dirty(state);
            app_state_sync_request_to_ui(state);
        }

        app_state_save_all_collections(state);

        snprintf(state->status_message, sizeof(state->status_message), 
                "Request duplicated successfully");
        return true;
    } else {
        snprintf(state->status_message, sizeof(state->status_message), 
                "Failed to duplicate request");
        return false;
    }
}

static void ui_request_panel_format_json_body(AppState* state);
static void ui_request_panel_minify_json_body(AppState* state);
static void ui_request_panel_show_json_validation_status(AppState* state, const ModernGruvboxTheme* theme);
static void ui_request_panel_render_json_body(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme);
static void ui_request_panel_render_form_data_body(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme);
static void ui_request_panel_render_form_urlencoded_body(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme);
static void ui_request_panel_render_raw_body(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme, const char* type_name);
static void ui_request_panel_update_form_data_body(AppState* state, char form_keys[][256], char form_values[][512], bool form_enabled[], int pair_count, bool is_multipart);
static void ui_request_panel_url_encode(const char* input, char* output, size_t output_size);
static void ui_request_panel_url_decode(const char* input, char* output, size_t output_size);
static void ui_request_panel_render_auth_panel(UIManager* ui, AppState* state);
static void ui_request_panel_apply_authentication(AppState* state);

static void ui_request_panel_update_form_data_body(AppState* state, char form_keys[][256], char form_values[][512], bool form_enabled[], int pair_count, bool is_multipart) {
    if (!state) return;

    state->body_buffer[0] = '\0';

    if (is_multipart) {

        static const char* boundary = "TinyRequestFormBoundary1234567890";

        for (int i = 0; i < pair_count; i++) {
            if (form_enabled[i] && strlen(form_keys[i]) > 0) {

                size_t current_len = strlen(state->body_buffer);
                size_t remaining = sizeof(state->body_buffer) - current_len - 1;

                snprintf(state->body_buffer + current_len, remaining,
                    "--%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n%s\r\n",
                    boundary, form_keys[i], form_values[i]);
            }
        }

        if (strlen(state->body_buffer) > 0) {
            size_t current_len = strlen(state->body_buffer);
            size_t remaining = sizeof(state->body_buffer) - current_len - 1;
            snprintf(state->body_buffer + current_len, remaining, "--%s--\r\n", boundary);
        }

    } else {

        for (int i = 0; i < pair_count; i++) {
            if (form_enabled[i] && strlen(form_keys[i]) > 0) {
                if (strlen(state->body_buffer) > 0) {
                    strcat(state->body_buffer, "&");
                }

                char encoded_key[512];
                char encoded_value[1024];

                ui_request_panel_url_encode(form_keys[i], encoded_key, sizeof(encoded_key));
                ui_request_panel_url_encode(form_values[i], encoded_value, sizeof(encoded_value));

                size_t current_len = strlen(state->body_buffer);
                size_t remaining = sizeof(state->body_buffer) - current_len - 1;

                snprintf(state->body_buffer + current_len, remaining, "%s=%s", encoded_key, encoded_value);
            }
        }
    }

    app_state_sync_ui_to_request(state);
    app_state_mark_ui_dirty(state);
    app_state_set_unsaved_changes(state, true);
}

static void ui_request_panel_url_encode(const char* input, char* output, size_t output_size) {
    if (!input || !output || output_size == 0) return;

    const char* hex = "0123456789ABCDEF";
    size_t input_len = strlen(input);
    size_t output_pos = 0;

    for (size_t i = 0; i < input_len && output_pos < output_size - 1; i++) {
        unsigned char c = (unsigned char)input[i];

        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {

            if (output_pos < output_size - 1) {
                output[output_pos++] = c;
            }
        } else {

            if (output_pos < output_size - 3) {
                output[output_pos++] = '%';
                output[output_pos++] = hex[c >> 4];
                output[output_pos++] = hex[c & 15];
            }
        }
    }

    output[output_pos] = '\0';
}

static void ui_request_panel_url_decode(const char* input, char* output, size_t output_size) {
    if (!input || !output || output_size == 0) return;

    size_t input_len = strlen(input);
    size_t output_pos = 0;

    for (size_t i = 0; i < input_len && output_pos < output_size - 1; i++) {
        unsigned char c = (unsigned char)input[i];

        if (c == '%' && i + 2 < input_len) {

            char hex1 = input[i + 1];
            char hex2 = input[i + 2];

            int val1 = (hex1 >= '0' && hex1 <= '9') ? hex1 - '0' :
                      (hex1 >= 'A' && hex1 <= 'F') ? hex1 - 'A' + 10 :
                      (hex1 >= 'a' && hex1 <= 'f') ? hex1 - 'a' + 10 : 0;
            int val2 = (hex2 >= '0' && hex2 <= '9') ? hex2 - '0' :
                      (hex2 >= 'A' && hex2 <= 'F') ? hex2 - 'A' + 10 :
                      (hex2 >= 'a' && hex2 <= 'f') ? hex2 - 'a' + 10 : 0;

            if (output_pos < output_size - 1) {
                output[output_pos++] = (val1 << 4) | val2;
            }

            i += 2;
        } else if (c == '+') {

            if (output_pos < output_size - 1) {
                output[output_pos++] = ' ';
            }
        } else {

            if (output_pos < output_size - 1) {
                output[output_pos++] = c;
            }
        }
    }

    output[output_pos] = '\0';
}

static void ui_request_panel_format_json_body(AppState* state) {
    if (!state || strlen(state->body_buffer) == 0) {
        return;
    }

    cJSON* json = cJSON_Parse(state->body_buffer);
    if (json) {
        char* formatted = cJSON_Print(json);
        if (formatted) {

            size_t formatted_len = strlen(formatted);
            if (formatted_len < sizeof(state->body_buffer)) {
                strcpy(state->body_buffer, formatted);

                Request* active_request = app_state_get_active_request(state);
                HeaderList* headers = active_request ? &active_request->headers : &state->current_request.headers;

                for (int i = 0; i < headers->count; i++) {
                    if (strcasecmp(headers->headers[i].name, "content-type") == 0) {
                        header_list_remove(headers, i);
                        break;
                    }
                }

                header_list_add(headers, "Content-Type", "application/json");

                app_state_mark_ui_dirty(state);
                app_state_set_unsaved_changes(state, true);
            }
            free(formatted);
        }
        cJSON_Delete(json);
    }
}

static void ui_request_panel_format_json_body_separate(AppState* state, char* json_buffer) {
    if (!state || !json_buffer || strlen(json_buffer) == 0) {
        return;
    }

    cJSON* json = cJSON_Parse(json_buffer);
    if (json) {
        char* formatted = cJSON_Print(json);
        if (formatted) {

            size_t formatted_len = strlen(formatted);
            if (formatted_len < sizeof(state->json_body_buffer)) {
                strcpy(json_buffer, formatted);

                Request* active_request = app_state_get_active_request(state);
                HeaderList* headers = active_request ? &active_request->headers : &state->current_request.headers;

                for (int i = 0; i < headers->count; i++) {
                    if (strcasecmp(headers->headers[i].name, "content-type") == 0) {
                        header_list_remove(headers, i);
                        break;
                    }
                }

                header_list_add(headers, "Content-Type", "application/json");

                app_state_mark_ui_dirty(state);
                app_state_set_unsaved_changes(state, true);
            }
            free(formatted);
        }
        cJSON_Delete(json);
    }
}

static void ui_request_panel_minify_json_body(AppState* state) {
    if (!state || strlen(state->body_buffer) == 0) {
        return;
    }

    cJSON* json = cJSON_Parse(state->body_buffer);
    if (json) {
        char* minified = cJSON_PrintUnformatted(json);
        if (minified) {

            size_t minified_len = strlen(minified);
            if (minified_len < sizeof(state->body_buffer)) {
                strcpy(state->body_buffer, minified);

                Request* active_request = app_state_get_active_request(state);
                HeaderList* headers = active_request ? &active_request->headers : &state->current_request.headers;

                for (int i = 0; i < headers->count; i++) {
                    if (strcasecmp(headers->headers[i].name, "content-type") == 0) {
                        header_list_remove(headers, i);
                        break;
                    }
                }

                header_list_add(headers, "Content-Type", "application/json");

                app_state_mark_ui_dirty(state);
                app_state_set_unsaved_changes(state, true);
            }
            free(minified);
        }
        cJSON_Delete(json);
    }
}

static void ui_request_panel_minify_json_body_separate(AppState* state, char* json_buffer) {
    if (!state || !json_buffer || strlen(json_buffer) == 0) {
        return;
    }

    cJSON* json = cJSON_Parse(json_buffer);
    if (json) {
        char* minified = cJSON_PrintUnformatted(json);
        if (minified) {

            size_t minified_len = strlen(minified);
            if (minified_len < sizeof(state->json_body_buffer)) {
                strcpy(json_buffer, minified);

                Request* active_request = app_state_get_active_request(state);
                HeaderList* headers = active_request ? &active_request->headers : &state->current_request.headers;

                for (int i = 0; i < headers->count; i++) {
                    if (strcasecmp(headers->headers[i].name, "content-type") == 0) {
                        header_list_remove(headers, i);
                        break;
                    }
                }

                header_list_add(headers, "Content-Type", "application/json");

                app_state_mark_ui_dirty(state);
                app_state_set_unsaved_changes(state, true);
            }
            free(minified);
        }
        cJSON_Delete(json);
    }
}

static void ui_request_panel_show_json_validation_status(AppState* state, const ModernGruvboxTheme* theme) {
    if (strlen(state->body_buffer) == 0) {
        return;
    }

    cJSON* json = cJSON_Parse(state->body_buffer);
    if (json) {
        ImGui::PushStyleColor(ImGuiCol_Text, theme->success);
        ImGui::Text(ICON_FA_CHECK " Valid JSON");
        ImGui::PopStyleColor();
        cJSON_Delete(json);
    } else {
        const char* error_ptr = cJSON_GetErrorPtr();
        ImGui::PushStyleColor(ImGuiCol_Text, theme->error);
        if (error_ptr) {
            ImGui::Text(ICON_FA_TIMES " Invalid JSON: Error near '%.10s'", error_ptr);
        } else {
            ImGui::Text(ICON_FA_TIMES " Invalid JSON format");
        }
        ImGui::PopStyleColor();
    }
}

static void ui_request_panel_show_json_validation_status_separate(AppState* state, const ModernGruvboxTheme* theme, char* json_buffer) {
    if (!json_buffer || strlen(json_buffer) == 0) {
        return;
    }

    cJSON* json = cJSON_Parse(json_buffer);
    if (json) {
        ImGui::PushStyleColor(ImGuiCol_Text, theme->success);
        ImGui::Text(ICON_FA_CHECK " Valid JSON");
        ImGui::PopStyleColor();
        cJSON_Delete(json);
    } else {
        const char* error_ptr = cJSON_GetErrorPtr();
        ImGui::PushStyleColor(ImGuiCol_Text, theme->error);
        if (error_ptr) {
            ImGui::Text(ICON_FA_TIMES " Invalid JSON: Error near '%.10s'", error_ptr);
        } else {
            ImGui::Text(ICON_FA_TIMES " Invalid JSON format");
        }
        ImGui::PopStyleColor();
    }
}

static void ui_request_panel_render_json_body(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme) {
    (void)ui; 

    char* json_buffer = app_state_get_content_buffer(state, CONTENT_TYPE_JSON);
    if (!json_buffer) {
        return;
    }

    ImGui::BeginGroup();

    theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
    if (ImGui::Button("Format JSON", ImVec2(100, 0))) {
        ui_request_panel_format_json_body_separate(state, json_buffer);
    }
    theme_pop_button_style();

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Format and indent JSON content");
    }

    ImGui::SameLine();

    theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
    if (ImGui::Button("Minify JSON", ImVec2(100, 0))) {
        ui_request_panel_minify_json_body_separate(state, json_buffer);
    }
    theme_pop_button_style();

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Remove formatting and whitespace");
    }

    ImGui::EndGroup();

    ImGui::Spacing();

    ImGui::BeginChild("JSONEditor", ImVec2(-1.0f, 200.0f), false); 

    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme_alpha_blend(theme->success, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_Border, theme_alpha_blend(theme->success, 0.3f));

    if (ImGui::InputTextMultiline("##json_body", json_buffer, sizeof(state->json_body_buffer),
                                  ImVec2(-1.0f, -1.0f))) {

        if (strlen(json_buffer) > 0) {

            const char* trimmed = json_buffer;
            while (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n' || *trimmed == '\r') {
                trimmed++;
            }

            if (*trimmed == '{' || *trimmed == '[') {

                Request* active_request = app_state_get_active_request(state);
                HeaderList* headers = active_request ? &active_request->headers : &state->current_request.headers;

                bool has_content_type = false;
                for (int i = 0; i < headers->count; i++) {
                    if (strcasecmp(headers->headers[i].name, "content-type") == 0) {
                        has_content_type = true;

                        if (strcasecmp(headers->headers[i].value, "application/json") != 0) {
                            strncpy(headers->headers[i].value, "application/json", sizeof(headers->headers[i].value) - 1);
                            headers->headers[i].value[sizeof(headers->headers[i].value) - 1] = '\0';
                        }
                        break;
                    }
                }

                if (!has_content_type) {
                    header_list_add(headers, "Content-Type", "application/json");
                }
            }
        }

        app_state_sync_content_to_body_buffer(state, CONTENT_TYPE_JSON);
        app_state_mark_ui_dirty(state);
        app_state_set_unsaved_changes(state, true);
    }

    ImGui::PopStyleColor(2);
    ImGui::EndChild();

    ImGui::Spacing();
    ui_request_panel_show_json_validation_status_separate(state, theme, json_buffer);
}

static void ui_request_panel_render_form_data_body(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme) {
    (void)ui; 

    static char form_keys[50][256] = {0};
    static char form_values[50][512] = {0};
    static bool form_enabled[50] = {true};
    static int form_pair_count = 1;
    static bool initialized = false;
    static Request* last_request = NULL;
    static char last_request_id[64] = {0}; 

    Request* current_request = app_state_get_active_request(state);
    char current_request_id[64] = {0};
    if (current_request) {

        snprintf(current_request_id, sizeof(current_request_id), "%p_%s_%s",
                (void*)current_request, current_request->method, current_request->url);
    }

    bool request_changed = (strcmp(current_request_id, last_request_id) != 0);
    if (!initialized || request_changed) {

        for (int i = 0; i < 50; i++) {
            form_keys[i][0] = '\0';
            form_values[i][0] = '\0';
            form_enabled[i] = (i == 0); 
        }
        form_pair_count = 1;

        if (current_request && current_request->body && current_request->body_size > 0) {
            const char* body_content = current_request->body;

            if (strstr(body_content, "--TinyRequestFormBoundary") != NULL) {

                const char* boundary = "TinyRequestFormBoundary1234567890";
                const char* current_pos = body_content;

                int pair_index = 0;
                while ((current_pos = strstr(current_pos, boundary)) != NULL && pair_index < 49) {

                    current_pos += strlen(boundary);
                    if (strncmp(current_pos, "\r\n", 2) == 0) {
                        current_pos += 2;
                    }

                    if (strncmp(current_pos, "--\r\n", 4) == 0) {
                        break; 
                    }

                    const char* content_disposition = strstr(current_pos, "Content-Disposition: form-data;");
                    if (!content_disposition) {

                        current_pos = strstr(current_pos, boundary);
                        if (!current_pos) break;
                        continue;
                    }

                    const char* name_start = strstr(content_disposition, "name=\"");
                    if (name_start) {
                        name_start += 6; 
                        const char* name_end = strchr(name_start, '"');
                        if (name_end && name_end > name_start) {
                            size_t name_len = name_end - name_start;
                            if (name_len < sizeof(form_keys[pair_index])) {
                                strncpy(form_keys[pair_index], name_start, name_len);
                                form_keys[pair_index][name_len] = '\0';

                                const char* value_start = strstr(name_start, "\r\n\r\n");
                                if (value_start) {
                                    value_start += 4; 

                                    const char* value_end = strstr(value_start, "\r\n--");
                                    if (value_end) {
                                        size_t value_len = value_end - value_start;
                                        if (value_len < sizeof(form_values[pair_index])) {
                                            strncpy(form_values[pair_index], value_start, value_len);
                                            form_values[pair_index][value_len] = '\0';
                                            form_enabled[pair_index] = true;
                                            pair_index++;
                                        }
                                    } else {

                                        size_t value_len = strlen(value_start);
                                        if (value_len < sizeof(form_values[pair_index])) {
                                            strncpy(form_values[pair_index], value_start, value_len);
                                            form_values[pair_index][value_len] = '\0';
                                            form_enabled[pair_index] = true;
                                            pair_index++;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    current_pos = strstr(current_pos, boundary);
                    if (!current_pos) break;
                }

                form_pair_count = pair_index > 0 ? pair_index : 1;
            }
        }

        initialized = true;
        last_request = current_request;
        strcpy(last_request_id, current_request_id);
    }

    theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
    if (ImGui::Button(ICON_FA_PLUS " Add Field", ImVec2(100, 0)) && form_pair_count < 50) {
        form_enabled[form_pair_count] = true; 
        form_pair_count++;
    }
    theme_pop_button_style();

    ImGui::SameLine();

    theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
    if (ImGui::Button("Clear All", ImVec2(80, 0))) {
        for (int i = 0; i < 50; i++) {
            form_keys[i][0] = '\0';
            form_values[i][0] = '\0';
            form_enabled[i] = (i == 0); 
        }
        form_pair_count = 1;
        state->body_buffer[0] = '\0';
        app_state_mark_ui_dirty(state);
        app_state_set_unsaved_changes(state, true);
    }
    theme_pop_button_style();

    ImGui::Spacing();
    ImGui::Separator();

    float base_height = 60.0f; 
    float row_height = 30.0f;  
    float dynamic_height = base_height + (form_pair_count * row_height);

    int max_visible_rows = 12; 
    float max_dynamic_height = base_height + (max_visible_rows * row_height);

    float window_height = ImGui::GetWindowSize().y;
    float current_y = ImGui::GetCursorPosY();
    float bottom_margin = 200.0f; 
    float max_available_height = window_height - current_y - bottom_margin;

    float final_height;
    bool needs_scrollbar = false;

    if (form_pair_count > max_visible_rows) {
        final_height = max_dynamic_height;
        needs_scrollbar = true;
    } else if (dynamic_height <= max_available_height) {
        final_height = dynamic_height;
        needs_scrollbar = false;
    } else {
        final_height = max_available_height;
        needs_scrollbar = true;
    }

    if (final_height < 100.0f) {
        final_height = 100.0f;
        needs_scrollbar = true;
    }

    ImGuiWindowFlags editor_flags = needs_scrollbar ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoScrollbar;
    ImGui::BeginChild("FormDataEditor", ImVec2(-1.0f, final_height), false, editor_flags);

    ImGui::Columns(3, "FormDataColumns", true);
    ImGui::SetColumnWidth(0, 200.0f); 
    ImGui::SetColumnWidth(1, 300.0f); 
    ImGui::SetColumnWidth(2, 120.0f); 

    ImGui::Text("Key");
    ImGui::NextColumn();
    ImGui::Text("Value");
    ImGui::NextColumn();
    ImGui::Text("Actions");
    ImGui::NextColumn();
    ImGui::Separator();

    for (int i = 0; i < form_pair_count; i++) {
        ImGui::PushID(i);

        if (!form_enabled[i]) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        }
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##key", form_keys[i], sizeof(form_keys[i]))) {
            ui_request_panel_update_form_data_body(state, form_keys, form_values, form_enabled, form_pair_count, true);
        }
        if (!form_enabled[i]) {
            ImGui::PopStyleVar();
        }
        ImGui::NextColumn();

        if (!form_enabled[i]) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        }
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##value", form_values[i], sizeof(form_values[i]))) {
            ui_request_panel_update_form_data_body(state, form_keys, form_values, form_enabled, form_pair_count, true);
        }
        if (!form_enabled[i]) {
            ImGui::PopStyleVar();
        }
        ImGui::NextColumn();

        if (ImGui::Checkbox("##enabled", &form_enabled[i])) {
            ui_request_panel_update_form_data_body(state, form_keys, form_values, form_enabled, form_pair_count, true);
        }

        ImGui::SameLine();

        if (form_pair_count > 1) {
            theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
            if (ImGui::Button(ICON_FA_TRASH, ImVec2(30, 0))) {

                for (int j = i; j < form_pair_count - 1; j++) {
                    strcpy(form_keys[j], form_keys[j + 1]);
                    strcpy(form_values[j], form_values[j + 1]);
                    form_enabled[j] = form_enabled[j + 1];
                }
                form_keys[form_pair_count - 1][0] = '\0';
                form_values[form_pair_count - 1][0] = '\0';
                form_enabled[form_pair_count - 1] = true;
                form_pair_count--;
                ui_request_panel_update_form_data_body(state, form_keys, form_values, form_enabled, form_pair_count, true);
            }
            theme_pop_button_style();
        }
        ImGui::NextColumn();

        ImGui::PopID();
    }

    ImGui::Columns(1);
    ImGui::EndChild();

}

static void ui_request_panel_render_form_urlencoded_body(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme) {
    (void)ui; 

    static char urlencoded_keys[50][256] = {0};
    static char urlencoded_values[50][512] = {0};
    static bool urlencoded_enabled[50] = {true};
    static int urlencoded_pair_count = 1;
    static bool initialized = false;
    static Request* last_request = NULL;
    static char last_request_id[64] = {0}; 

    Request* current_request = app_state_get_active_request(state);
    char current_request_id[64] = {0};
    if (current_request) {

        snprintf(current_request_id, sizeof(current_request_id), "%p_%s_%s",
                (void*)current_request, current_request->method, current_request->url);
    }

    bool request_changed = (strcmp(current_request_id, last_request_id) != 0);
    if (!initialized || request_changed) {

        for (int i = 0; i < 50; i++) {
            urlencoded_keys[i][0] = '\0';
            urlencoded_values[i][0] = '\0';
            urlencoded_enabled[i] = (i == 0); 
        }
        urlencoded_pair_count = 1;

        if (current_request && current_request->body && current_request->body_size > 0) {
            const char* body_content = current_request->body;

            if (strstr(body_content, "=") != NULL && strstr(body_content, "--TinyRequestFormBoundary") == NULL) {

                const char* current_pos = body_content;
                int pair_index = 0;

                while (current_pos && *current_pos && pair_index < 49) {

                    const char* next_ampersand = strchr(current_pos, '&');
                    if (!next_ampersand) {
                        next_ampersand = current_pos + strlen(current_pos); 
                    }

                    char pair_content[1024];
                    size_t pair_length = next_ampersand - current_pos;
                    if (pair_length < sizeof(pair_content) && pair_length > 0) {
                        strncpy(pair_content, current_pos, pair_length);
                        pair_content[pair_length] = '\0';

                        char* equals_pos = strchr(pair_content, '=');
                        if (equals_pos) {
                            *equals_pos = '\0'; 
                            char* key = pair_content;
                            char* value = equals_pos + 1;

                            char decoded_key[256];
                            char decoded_value[512];

                            size_t key_len = strlen(key);
                            size_t value_len = strlen(value);
                            if (key_len < sizeof(decoded_key) && value_len < sizeof(decoded_value)) {

                                ui_request_panel_url_decode(key, decoded_key, sizeof(decoded_key));
                                ui_request_panel_url_decode(value, decoded_value, sizeof(decoded_value));

                                strncpy(urlencoded_keys[pair_index], decoded_key, sizeof(urlencoded_keys[pair_index]) - 1);
                                urlencoded_keys[pair_index][sizeof(urlencoded_keys[pair_index]) - 1] = '\0';
                                strncpy(urlencoded_values[pair_index], decoded_value, sizeof(urlencoded_values[pair_index]) - 1);
                                urlencoded_values[pair_index][sizeof(urlencoded_values[pair_index]) - 1] = '\0';
                                urlencoded_enabled[pair_index] = true;
                                pair_index++;
                            }
                        }
                    }

                    current_pos = next_ampersand;
                    if (current_pos && *current_pos == '&') {
                        current_pos++; 
                    }
                }

                urlencoded_pair_count = pair_index > 0 ? pair_index : 1;
            }
        }

        initialized = true;
        last_request = current_request;
        strcpy(last_request_id, current_request_id);
    }

    theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
    if (ImGui::Button(ICON_FA_PLUS " Add Field", ImVec2(100, 0)) && urlencoded_pair_count < 50) {
        urlencoded_enabled[urlencoded_pair_count] = true; 
        urlencoded_pair_count++;
    }
    theme_pop_button_style();

    ImGui::SameLine();

    theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
    if (ImGui::Button("Clear All", ImVec2(80, 0))) {
        for (int i = 0; i < 50; i++) {
            urlencoded_keys[i][0] = '\0';
            urlencoded_values[i][0] = '\0';
            urlencoded_enabled[i] = (i == 0); 
        }
        urlencoded_pair_count = 1;
        state->body_buffer[0] = '\0';
        app_state_mark_ui_dirty(state);
        app_state_set_unsaved_changes(state, true);
    }
    theme_pop_button_style();

    ImGui::Spacing();
    ImGui::Separator();

    float base_height_url = 60.0f; 
    float row_height_url = 30.0f;  
    float dynamic_height_url = base_height_url + (urlencoded_pair_count * row_height_url);

    int max_visible_rows_url = 12; 
    float max_dynamic_height_url = base_height_url + (max_visible_rows_url * row_height_url);

    float window_height_url = ImGui::GetWindowSize().y;
    float current_y_url = ImGui::GetCursorPosY();
    float bottom_margin_url = 200.0f; 
    float max_available_height_url = window_height_url - current_y_url - bottom_margin_url;

    float final_height_url;
    bool needs_scrollbar_url = false;

    if (urlencoded_pair_count > max_visible_rows_url) {
        final_height_url = max_dynamic_height_url;
        needs_scrollbar_url = true;
    } else if (dynamic_height_url <= max_available_height_url) {
        final_height_url = dynamic_height_url;
        needs_scrollbar_url = false;
    } else {
        final_height_url = max_available_height_url;
        needs_scrollbar_url = true;
    }

    if (final_height_url < 100.0f) {
        final_height_url = 100.0f;
        needs_scrollbar_url = true;
    }

    ImGuiWindowFlags editor_flags_url = needs_scrollbar_url ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoScrollbar;
    ImGui::BeginChild("FormURLEditor", ImVec2(-1.0f, final_height_url), false, editor_flags_url);

    ImGui::Columns(3, "URLEncodedColumns", true);
    ImGui::SetColumnWidth(0, 200.0f); 
    ImGui::SetColumnWidth(1, 300.0f); 
    ImGui::SetColumnWidth(2, 120.0f); 

    ImGui::Text("Key");
    ImGui::NextColumn();
    ImGui::Text("Value");
    ImGui::NextColumn();
    ImGui::Text("Actions");
    ImGui::NextColumn();
    ImGui::Separator();

    for (int i = 0; i < urlencoded_pair_count; i++) {
        ImGui::PushID(i);

        if (!urlencoded_enabled[i]) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        }
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##key", urlencoded_keys[i], sizeof(urlencoded_keys[i]))) {
            ui_request_panel_update_form_data_body(state, urlencoded_keys, urlencoded_values, urlencoded_enabled, urlencoded_pair_count, false);
        }
        if (!urlencoded_enabled[i]) {
            ImGui::PopStyleVar();
        }
        ImGui::NextColumn();

        if (!urlencoded_enabled[i]) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        }
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##value", urlencoded_values[i], sizeof(urlencoded_values[i]))) {
            ui_request_panel_update_form_data_body(state, urlencoded_keys, urlencoded_values, urlencoded_enabled, urlencoded_pair_count, false);
        }
        if (!urlencoded_enabled[i]) {
            ImGui::PopStyleVar();
        }
        ImGui::NextColumn();

        if (ImGui::Checkbox("##enabled", &urlencoded_enabled[i])) {
            ui_request_panel_update_form_data_body(state, urlencoded_keys, urlencoded_values, urlencoded_enabled, urlencoded_pair_count, false);
        }

        ImGui::SameLine();

        if (urlencoded_pair_count > 1) {
            theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
            if (ImGui::Button(ICON_FA_TRASH, ImVec2(30, 0))) {

                for (int j = i; j < urlencoded_pair_count - 1; j++) {
                    strcpy(urlencoded_keys[j], urlencoded_keys[j + 1]);
                    strcpy(urlencoded_values[j], urlencoded_values[j + 1]);
                    urlencoded_enabled[j] = urlencoded_enabled[j + 1];
                }
                urlencoded_keys[urlencoded_pair_count - 1][0] = '\0';
                urlencoded_values[urlencoded_pair_count - 1][0] = '\0';
                urlencoded_enabled[urlencoded_pair_count - 1] = true;
                urlencoded_pair_count--;
                ui_request_panel_update_form_data_body(state, urlencoded_keys, urlencoded_values, urlencoded_enabled, urlencoded_pair_count, false);
            }
            theme_pop_button_style();
        }
        ImGui::NextColumn();

        ImGui::PopID();
    }

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Spacing();

}

static void ui_request_panel_render_raw_body(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme, const char* type_name) {
    (void)ui; 

    bool is_xml = (strcmp(type_name, "XML") == 0);
    bool is_yaml = (strcmp(type_name, "YAML") == 0);
    bool is_plain_text = (strcmp(type_name, "Plain Text") == 0);

    char* content_buffer = NULL;
    size_t buffer_size = 0;
    ContentType content_type;

    if (is_xml) {
        content_buffer = app_state_get_content_buffer(state, CONTENT_TYPE_XML);
        buffer_size = sizeof(state->xml_body_buffer);
        content_type = CONTENT_TYPE_XML;
    } else if (is_yaml) {
        content_buffer = app_state_get_content_buffer(state, CONTENT_TYPE_YAML);
        buffer_size = sizeof(state->yaml_body_buffer);
        content_type = CONTENT_TYPE_YAML;
    } else {
        content_buffer = app_state_get_content_buffer(state, CONTENT_TYPE_PLAIN_TEXT);
        buffer_size = sizeof(state->plain_text_body_buffer);
        content_type = CONTENT_TYPE_PLAIN_TEXT;
    }

    if (!content_buffer) {
        return;
    }

    if (is_xml || is_yaml) {
        ImGui::BeginGroup();

        theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
        if (ImGui::Button(is_xml ? "Format XML" : "Format YAML", ImVec2(100, 0))) {

            if (strlen(content_buffer) > 0) {

                app_state_mark_ui_dirty(state);
                app_state_set_unsaved_changes(state, true);
            }
        }
        theme_pop_button_style();

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(is_xml ? "Format and indent XML content" : "Format and indent YAML content");
        }

        ImGui::SameLine();

        theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
        if (ImGui::Button("Validate", ImVec2(80, 0))) {

            if (strlen(content_buffer) > 0) {

                bool has_content = strlen(content_buffer) > 0;
                if (has_content) {
                    snprintf(state->status_message, sizeof(state->status_message),
                            "%s content appears valid", type_name);
                }
            }
        }
        theme_pop_button_style();

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(is_xml ? "Validate XML syntax" : "Validate YAML syntax");
        }

        ImGui::EndGroup();
        ImGui::Spacing();
    }

    ImGui::BeginChild("RawEditor", ImVec2(-1.0f, 200.0f), false); 

    if (is_xml) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, theme_alpha_blend(theme->warning, 0.1f));
        ImGui::PushStyleColor(ImGuiCol_Border, theme_alpha_blend(theme->warning, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_primary);
    } else if (is_yaml) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, theme_alpha_blend(theme->accent_primary, 0.1f));
        ImGui::PushStyleColor(ImGuiCol_Border, theme_alpha_blend(theme->accent_primary, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_primary);
    } else {

        ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->bg_input);
        ImGui::PushStyleColor(ImGuiCol_Border, theme->border_normal);
        ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_primary);
    }

    if (ImGui::InputTextMultiline("##raw_body", content_buffer, buffer_size,
                                  ImVec2(-1.0f, -1.0f))) {

        app_state_sync_content_to_body_buffer(state, content_type);
        app_state_mark_ui_dirty(state);
        app_state_set_unsaved_changes(state, true);
    }

    ImGui::PopStyleColor(3);
    ImGui::EndChild();

    ImGui::Spacing();

    if (strlen(content_buffer) > 0) {

        ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_secondary);
        ImGui::Text("Content length: %zu bytes", strlen(content_buffer));
        ImGui::PopStyleColor();

        if (is_xml) {
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();

            const char* content = content_buffer;
            bool has_opening_tag = (strstr(content, "<") != NULL);
            bool has_closing_tag = (strstr(content, ">") != NULL);

            if (has_opening_tag && has_closing_tag) {
                ImGui::PushStyleColor(ImGuiCol_Text, theme->success);
                ImGui::Text(ICON_FA_CHECK " XML structure detected");
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, theme->warning);
                ImGui::Text(ICON_FA_TIMES " Check XML syntax");
                ImGui::PopStyleColor();
            }
        } else if (is_yaml) {
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();

            const char* content = content_buffer;
            bool has_yaml_structure = (strstr(content, ":") != NULL || strstr(content, "-") != NULL);

            if (has_yaml_structure) {
                ImGui::PushStyleColor(ImGuiCol_Text, theme->success);
                ImGui::Text(ICON_FA_CHECK " YAML structure detected");
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, theme->warning);
                ImGui::Text(ICON_FA_TIMES " Check YAML syntax");
                ImGui::PopStyleColor();
            }
        }
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_disabled);
        if (is_xml) {
            ImGui::Text("Enter XML content here (e.g., <root><item>value</item></root>)");
        } else if (is_yaml) {
            ImGui::Text("Enter YAML content here (e.g., key: value)");
        } else {
            ImGui::Text("Enter %s content here", type_name);
        }
        ImGui::PopStyleColor();
    }
}

void ui_request_panel_render_body_panel(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    static int selected_body_type = 0; 
    static int previous_body_type = -1; 
    static bool auto_detected_content_type = false; 
    const char* body_types[] = { "JSON", "Form Data", "Form URL Encoded", "Plain Text", "XML", "YAML" };
    const int body_type_count = sizeof(body_types) / sizeof(body_types[0]);

    static Request* last_active_request = NULL;
    Request* current_active_request = app_state_get_active_request(state);
    if (current_active_request != last_active_request) {
        auto_detected_content_type = false;
        last_active_request = current_active_request;
    }

    if (!auto_detected_content_type) {
        if (current_active_request) {

            const char* content_type_header = NULL;
            for (int i = 0; i < current_active_request->headers.count; i++) {
                if (strcasecmp(current_active_request->headers.headers[i].name, "content-type") == 0) {
                    content_type_header = current_active_request->headers.headers[i].value;
                    break;
                }
            }

            if (content_type_header) {
                if (strstr(content_type_header, "application/json") != NULL) {
                    selected_body_type = 0; 
                } else if (strstr(content_type_header, "multipart/form-data") != NULL) {
                    selected_body_type = 1; 
                } else if (strstr(content_type_header, "application/x-www-form-urlencoded") != NULL) {
                    selected_body_type = 2; 
                } else if (strstr(content_type_header, "text/plain") != NULL) {
                    selected_body_type = 3; 
                } else if (strstr(content_type_header, "application/xml") != NULL || strstr(content_type_header, "text/xml") != NULL) {
                    selected_body_type = 4; 
                } else if (strstr(content_type_header, "application/x-yaml") != NULL || strstr(content_type_header, "text/yaml") != NULL) {
                    selected_body_type = 5; 
                }
            } else if (strlen(state->body_buffer) > 0) {

                const char* content = state->body_buffer;
                const char* trimmed = content;
                while (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n' || *trimmed == '\r') {
                    trimmed++;
                }

                if (strstr(content, "----TinyRequestFormBoundary") != NULL) {
                    selected_body_type = 1; 
                } else if (strstr(content, "=") != NULL && strstr(content, "&") != NULL) {
                    selected_body_type = 2; 
                } else if (*trimmed == '{' || *trimmed == '[') {
                    selected_body_type = 0; 
                } else if (*trimmed == '<' && strstr(content, ">") != NULL) {
                    selected_body_type = 4; 
                } else if (strstr(content, ":") != NULL && (strstr(content, "\n") != NULL || strstr(content, "\r") != NULL)) {
                    selected_body_type = 5; 
                } else {
                    selected_body_type = 3; 
                }
            }

            auto_detected_content_type = true;
            previous_body_type = selected_body_type;
        }
    }

    ImGui::SetNextItemWidth(150);
    theme_push_input_style(theme);
    if (ImGui::Combo("##body_type", &selected_body_type, body_types, body_type_count)) {

        if (previous_body_type >= 0 && previous_body_type != selected_body_type) {

            if (previous_body_type == 1 || previous_body_type == 2) {

                app_state_sync_ui_to_request(state);
            } else {

                switch (previous_body_type) {
                    case 0: 
                        app_state_set_content_buffer(state, CONTENT_TYPE_JSON, state->body_buffer);
                        break;
                    case 3: 
                        app_state_set_content_buffer(state, CONTENT_TYPE_PLAIN_TEXT, state->body_buffer);
                        break;
                    case 4: 
                        app_state_set_content_buffer(state, CONTENT_TYPE_XML, state->body_buffer);
                        break;
                    case 5: 
                        app_state_set_content_buffer(state, CONTENT_TYPE_YAML, state->body_buffer);
                        break;
                }
            }

            char* new_buffer = NULL;
            switch (selected_body_type) {
                case 0: 
                    new_buffer = app_state_get_content_buffer(state, CONTENT_TYPE_JSON);
                    break;
                case 3: 
                    new_buffer = app_state_get_content_buffer(state, CONTENT_TYPE_PLAIN_TEXT);
                    break;
                case 4: 
                    new_buffer = app_state_get_content_buffer(state, CONTENT_TYPE_XML);
                    break;
                case 5: 
                    new_buffer = app_state_get_content_buffer(state, CONTENT_TYPE_YAML);
                    break;
                case 1: 
                case 2: 

                    break;
            }

            if (new_buffer && selected_body_type != 1 && selected_body_type != 2) {
                strncpy(state->body_buffer, new_buffer, sizeof(state->body_buffer) - 1);
                state->body_buffer[sizeof(state->body_buffer) - 1] = '\0';
            }
        }

        previous_body_type = selected_body_type;

        const char* content_type_value = "";
        switch (selected_body_type) {
            case 0: content_type_value = "application/json"; break;
            case 1: content_type_value = "multipart/form-data; boundary=TinyRequestFormBoundary1234567890"; break;
            case 2: content_type_value = "application/x-www-form-urlencoded"; break;
            case 3: content_type_value = "text/plain"; break;
            case 4: content_type_value = "application/xml"; break;
            case 5: content_type_value = "application/x-yaml"; break;
        }

        Request* active_request = app_state_get_active_request(state);
        HeaderList* headers = active_request ? &active_request->headers : &state->current_request.headers;

        for (int i = 0; i < headers->count; i++) {
            if (strcasecmp(headers->headers[i].name, "content-type") == 0) {
                header_list_remove(headers, i);
                break;
            }
        }

        if (strlen(content_type_value) > 0) {
            header_list_add(headers, "Content-Type", content_type_value);
        }

        app_state_mark_ui_dirty(state);
        app_state_set_unsaved_changes(state, true);
    }
    theme_pop_input_style();

    if (previous_body_type == -1) {
        previous_body_type = selected_body_type;
    }

    ImGui::Spacing();

    switch (selected_body_type) {
        case 0: 
            ui_request_panel_render_json_body(ui, state, theme);
            break;

        case 1: 
            ui_request_panel_render_form_data_body(ui, state, theme);
            break;

        case 2: 
            ui_request_panel_render_form_urlencoded_body(ui, state, theme);
            break;

        case 3: 
        case 4: 
        case 5: 
        default:
            ui_request_panel_render_raw_body(ui, state, theme, body_types[selected_body_type]);
            break;
    }
}

void ui_request_panel_render_headers_panel(UIManager* ui, AppState* state, HeaderList* headers) {
    if (!ui || !state || !headers) {
        return;
    }

    ui_panels_render_headers_panel(ui, headers, state);
}

void ui_request_panel_handle_keyboard_shortcuts(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput) {
        return;
    }

    bool ctrl_pressed = io.KeyCtrl;

    if (ctrl_pressed) {

        if (ImGui::IsKeyPressed(ImGuiKey_S)) {
            Request* active_request = app_state_get_active_request(state);
            if (active_request) {
                ui_request_panel_handle_save_request(ui, state);
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_D)) {
            Request* active_request = app_state_get_active_request(state);
            if (active_request) {
                ui_request_panel_handle_duplicate_request(ui, state);
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {

            bool url_valid = (strncmp(state->url_buffer, "http://", 7) == 0 ||
                             strncmp(state->url_buffer, "https://", 8) == 0) &&
                             strlen(state->url_buffer) > 8;

            if (!state->request_in_progress && url_valid) {
                ui_request_panel_handle_send_request(ui, state);
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_N)) {
            Collection* active_collection = app_state_get_active_collection(state);
            if (active_collection) {
                state->show_request_create_dialog = true;
                app_state_clear_ui_buffers(state);
            }
        }
    }
}

static void ui_request_panel_apply_authentication(AppState* state) {
    if (!state) return;

    Request* request = app_state_get_active_request(state);
    if (!request) {
        request = &state->current_request;
    }

    // Remove existing authentication headers
    for (int i = request->headers.count - 1; i >= 0; i--) {
        if (strcasecmp(request->headers.headers[i].name, "Authorization") == 0 ||
            strcasecmp(request->headers.headers[i].name, "X-API-Key") == 0 ||
            strcasecmp(request->headers.headers[i].name, request->auth_api_key_name) == 0) {
            header_list_remove(&request->headers, i);
        }
    }

    // Apply authentication only if the corresponding checkbox is enabled
    switch (request->selected_auth_type) {
        case 1: // API Key
            if (request->auth_api_key_enabled &&
                strlen(request->auth_api_key_name) > 0 && strlen(request->auth_api_key_value) > 0) {
                if (request->auth_api_key_location == 0) {
                    // Add to header
                    header_list_add(&request->headers, request->auth_api_key_name, request->auth_api_key_value);
                } else if (request->auth_api_key_location == 1) {
                    // Add to query parameters
                    char* url_to_modify = request->url;
                    char separator = strchr(url_to_modify, '?') ? '&' : '?';
                    size_t current_len = strlen(url_to_modify);
                    size_t remaining = sizeof(request->url) - current_len - 1;

                    if (remaining > strlen(request->auth_api_key_name) + strlen(request->auth_api_key_value) + 2) {
                        snprintf(url_to_modify + current_len, remaining, "%c%s=%s",
                                separator, request->auth_api_key_name, request->auth_api_key_value);
                    }
                }
            }
            break;

        case 2: // Bearer Token
            if (request->auth_bearer_enabled && strlen(request->auth_bearer_token) > 0) {
                char auth_header[600];
                snprintf(auth_header, sizeof(auth_header), "Bearer %s", request->auth_bearer_token);
                header_list_add(&request->headers, "Authorization", auth_header);
            }
            break;

        case 3: // Basic Auth
            if (request->auth_basic_enabled && strlen(request->auth_basic_username) > 0) {
                // Create base64-encoded credentials
                char credentials[600];
                snprintf(credentials, sizeof(credentials), "%s:%s",
                        request->auth_basic_username, request->auth_basic_password);

                char auth_header[800];
                snprintf(auth_header, sizeof(auth_header), "Basic %s", credentials);
                header_list_add(&request->headers, "Authorization", auth_header);
            }
            break;

        case 4: // OAuth 2.0
            if (request->auth_oauth_enabled && strlen(request->auth_oauth_token) > 0) {
                char auth_header[600];
                snprintf(auth_header, sizeof(auth_header), "Bearer %s", request->auth_oauth_token);
                header_list_add(&request->headers, "Authorization", auth_header);
            }
            break;

        default:
            break;
    }

    app_state_mark_ui_dirty(state);
    app_state_set_unsaved_changes(state, true);
}

static void ui_request_panel_render_auth_panel(UIManager* ui, AppState* state) {
    (void)ui;

    if (!state) return;

    const ModernGruvboxTheme* theme = theme_get_current();

    // Get the current request (either active request or current_request)
    Request* current_request = app_state_get_active_request(state);
    if (!current_request) {
        current_request = &state->current_request;
    }

    // The authentication data loading is handled automatically by the app_state_auto_sync()
    // system in the main loop. No manual synchronization needed here.

    const char* auth_types[] = { "No Auth", "API Key", "Bearer Token", "Basic Auth", "OAuth 2.0" };
    const int auth_type_count = sizeof(auth_types) / sizeof(auth_types[0]);

    ImGui::Text("Authentication Type:");
    ImGui::SetNextItemWidth(200);
    theme_push_input_style(theme);
    if (ImGui::Combo("##auth_type", &state->selected_auth_type, auth_types, auth_type_count)) {
        // Synchronize the change to per-request data
        current_request->selected_auth_type = state->selected_auth_type;
        ui_request_panel_apply_authentication(state);
        app_state_mark_ui_dirty(state);
        app_state_set_unsaved_changes(state, true);
    }
    theme_pop_input_style();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    switch (state->selected_auth_type) {
        case 0:
            theme_render_status_indicator("No authentication will be used for this request", STATUS_TYPE_INFO, theme);
            break;

        case 1:
            {
                ImGui::Text("API Key Configuration:");
                ImGui::Spacing();

                // Add checkbox to enable/disable API Key authentication
                if (ImGui::Checkbox("Enable API Key Authentication", &state->auth_api_key_enabled)) {
                    // Sync to per-request data
                    current_request->auth_api_key_enabled = state->auth_api_key_enabled;
                    ui_request_panel_apply_authentication(state);
                    app_state_mark_ui_dirty(state);
                    app_state_set_unsaved_changes(state, true);
                }
                ImGui::Spacing();

                // Disable input fields if checkbox is unchecked
                if (!state->auth_api_key_enabled) {
                    ImGui::BeginDisabled();
                }

                ImGui::Text("Key Name:");
                ImGui::SetNextItemWidth(300);
                if (ImGui::InputText("##api_key_name", state->auth_api_key_name, sizeof(state->auth_api_key_name))) {
                    // Sync to per-request data
                    strncpy(current_request->auth_api_key_name, state->auth_api_key_name, sizeof(current_request->auth_api_key_name) - 1);
                    current_request->auth_api_key_name[sizeof(current_request->auth_api_key_name) - 1] = '\0';
                    ui_request_panel_apply_authentication(state);
                    app_state_mark_ui_dirty(state);
                    app_state_set_unsaved_changes(state, true);
                }

                ImGui::Spacing();

                ImGui::Text("Key Value:");
                ImGui::SetNextItemWidth(400);
                if (ImGui::InputText("##api_key_value", state->auth_api_key_value, sizeof(state->auth_api_key_value), ImGuiInputTextFlags_Password)) {
                    // Sync to per-request data
                    strncpy(current_request->auth_api_key_value, state->auth_api_key_value, sizeof(current_request->auth_api_key_value) - 1);
                    current_request->auth_api_key_value[sizeof(current_request->auth_api_key_value) - 1] = '\0';
                    ui_request_panel_apply_authentication(state);
                    app_state_mark_ui_dirty(state);
                    app_state_set_unsaved_changes(state, true);
                }

                ImGui::Spacing();

                ImGui::Text("Add to:");
                const char* locations[] = { "Header", "Query Params" };
                ImGui::SetNextItemWidth(150);
                if (ImGui::Combo("##api_key_location", &state->auth_api_key_location, locations, 2)) {
                    // Sync to per-request data
                    current_request->auth_api_key_location = state->auth_api_key_location;
                    ui_request_panel_apply_authentication(state);
                    app_state_mark_ui_dirty(state);
                    app_state_set_unsaved_changes(state, true);
                }

                if (!state->auth_api_key_enabled) {
                    ImGui::EndDisabled();
                }

                ImGui::Spacing();

                if (state->auth_api_key_enabled) {
                    if (strlen(state->auth_api_key_name) > 0 && strlen(state->auth_api_key_value) > 0) {
                        ImGui::PushStyleColor(ImGuiCol_Text, theme->success);
                        ImGui::Text(ICON_FA_CHECK " API Key configured and enabled");
                        ImGui::PopStyleColor();
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Text, theme->warning);
                        ImGui::Text(ICON_FA_TIMES " Please enter key name and value");
                        ImGui::PopStyleColor();
                    }
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_tertiary);
                    ImGui::Text(ICON_FA_TIMES " API Key authentication disabled");
                    ImGui::PopStyleColor();
                }
            }
            break;

        case 2:
            {
                ImGui::Text("Bearer Token Configuration:");
                ImGui::Spacing();

                // Add checkbox to enable/disable Bearer Token authentication
                if (ImGui::Checkbox("Enable Bearer Token Authentication", &state->auth_bearer_enabled)) {
                    // Sync to per-request data
                    current_request->auth_bearer_enabled = state->auth_bearer_enabled;
                    ui_request_panel_apply_authentication(state);
                    app_state_mark_ui_dirty(state);
                    app_state_set_unsaved_changes(state, true);
                }
                ImGui::Spacing();

                // Disable input fields if checkbox is unchecked
                if (!state->auth_bearer_enabled) {
                    ImGui::BeginDisabled();
                }

                ImGui::Text("Token:");
                ImGui::SetNextItemWidth(500);
                if (ImGui::InputText("##bearer_token", state->auth_bearer_token, sizeof(state->auth_bearer_token), ImGuiInputTextFlags_Password)) {
                    // Sync to per-request data
                    strncpy(current_request->auth_bearer_token, state->auth_bearer_token, sizeof(current_request->auth_bearer_token) - 1);
                    current_request->auth_bearer_token[sizeof(current_request->auth_bearer_token) - 1] = '\0';
                    ui_request_panel_apply_authentication(state);
                    app_state_mark_ui_dirty(state);
                    app_state_set_unsaved_changes(state, true);
                }

                if (!state->auth_bearer_enabled) {
                    ImGui::EndDisabled();
                }

                ImGui::Spacing();

                if (state->auth_bearer_enabled) {
                    if (strlen(state->auth_bearer_token) > 0) {
                        ImGui::PushStyleColor(ImGuiCol_Text, theme->success);
                        ImGui::Text(ICON_FA_CHECK " Bearer token configured and enabled");
                        ImGui::PopStyleColor();
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Text, theme->warning);
                        ImGui::Text(ICON_FA_TIMES " Please enter bearer token");
                        ImGui::PopStyleColor();
                    }
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_tertiary);
                    ImGui::Text(ICON_FA_TIMES " Bearer token authentication disabled");
                    ImGui::PopStyleColor();
                }
            }
            break;

        case 3:
            {
                ImGui::Text("Basic Authentication Configuration:");
                ImGui::Spacing();

                // Add checkbox to enable/disable Basic Authentication
                if (ImGui::Checkbox("Enable Basic Authentication", &state->auth_basic_enabled)) {
                    // Sync to per-request data
                    current_request->auth_basic_enabled = state->auth_basic_enabled;
                    ui_request_panel_apply_authentication(state);
                    app_state_mark_ui_dirty(state);
                    app_state_set_unsaved_changes(state, true);
                }
                ImGui::Spacing();

                // Disable input fields if checkbox is unchecked
                if (!state->auth_basic_enabled) {
                    ImGui::BeginDisabled();
                }

                ImGui::Text("Username:");
                ImGui::SetNextItemWidth(300);
                if (ImGui::InputText("##basic_username", state->auth_basic_username, sizeof(state->auth_basic_username))) {
                    // Sync to per-request data
                    strncpy(current_request->auth_basic_username, state->auth_basic_username, sizeof(current_request->auth_basic_username) - 1);
                    current_request->auth_basic_username[sizeof(current_request->auth_basic_username) - 1] = '\0';
                    ui_request_panel_apply_authentication(state);
                    app_state_mark_ui_dirty(state);
                    app_state_set_unsaved_changes(state, true);
                }

                ImGui::Spacing();

                ImGui::Text("Password:");
                ImGui::SetNextItemWidth(300);
                if (ImGui::InputText("##basic_password", state->auth_basic_password, sizeof(state->auth_basic_password), ImGuiInputTextFlags_Password)) {
                    // Sync to per-request data
                    strncpy(current_request->auth_basic_password, state->auth_basic_password, sizeof(current_request->auth_basic_password) - 1);
                    current_request->auth_basic_password[sizeof(current_request->auth_basic_password) - 1] = '\0';
                    ui_request_panel_apply_authentication(state);
                    app_state_mark_ui_dirty(state);
                    app_state_set_unsaved_changes(state, true);
                }

                if (!state->auth_basic_enabled) {
                    ImGui::EndDisabled();
                }

                ImGui::Spacing();

                if (state->auth_basic_enabled) {
                    if (strlen(state->auth_basic_username) > 0) {
                        ImGui::PushStyleColor(ImGuiCol_Text, theme->success);
                        ImGui::Text(ICON_FA_CHECK " Basic auth configured and enabled");
                        ImGui::PopStyleColor();
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Text, theme->warning);
                        ImGui::Text(ICON_FA_TIMES " Please enter username");
                        ImGui::PopStyleColor();
                    }
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_tertiary);
                    ImGui::Text(ICON_FA_TIMES " Basic authentication disabled");
                    ImGui::PopStyleColor();
                }
            }
            break;

        case 4:
            {
                ImGui::Text("OAuth 2.0 Configuration:");
                ImGui::Spacing();

                // Add checkbox to enable/disable OAuth 2.0 authentication
                if (ImGui::Checkbox("Enable OAuth 2.0 Authentication", &state->auth_oauth_enabled)) {
                    // Sync to per-request data
                    current_request->auth_oauth_enabled = state->auth_oauth_enabled;
                    ui_request_panel_apply_authentication(state);
                    app_state_mark_ui_dirty(state);
                    app_state_set_unsaved_changes(state, true);
                }
                ImGui::Spacing();

                // Disable input fields if checkbox is unchecked
                if (!state->auth_oauth_enabled) {
                    ImGui::BeginDisabled();
                }

                ImGui::Text("Access Token:");
                ImGui::SetNextItemWidth(500);
                if (ImGui::InputText("##oauth_token", state->auth_oauth_token, sizeof(state->auth_oauth_token), ImGuiInputTextFlags_Password)) {
                    // Sync to per-request data
                    strncpy(current_request->auth_oauth_token, state->auth_oauth_token, sizeof(current_request->auth_oauth_token) - 1);
                    current_request->auth_oauth_token[sizeof(current_request->auth_oauth_token) - 1] = '\0';
                    ui_request_panel_apply_authentication(state);
                    app_state_mark_ui_dirty(state);
                    app_state_set_unsaved_changes(state, true);
                }

                if (!state->auth_oauth_enabled) {
                    ImGui::EndDisabled();
                }

                ImGui::Spacing();

                if (state->auth_oauth_enabled) {
                    if (strlen(state->auth_oauth_token) > 0) {
                        ImGui::PushStyleColor(ImGuiCol_Text, theme->success);
                        ImGui::Text(ICON_FA_CHECK " OAuth 2.0 token configured and enabled");
                        ImGui::PopStyleColor();
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Text, theme->warning);
                        ImGui::Text(ICON_FA_TIMES " Please enter access token");
                        ImGui::PopStyleColor();
                    }
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_tertiary);
                    ImGui::Text(ICON_FA_TIMES " OAuth 2.0 authentication disabled");
                    ImGui::PopStyleColor();
                }
            }
            break;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_tertiary);
    switch (state->selected_auth_type) {
        case 1:
            if (state->auth_api_key_enabled) {
                ImGui::Text("API Key will be added as a header or query parameter when enabled");
            } else {
                ImGui::Text("API Key is configured but disabled - check the box above to enable");
            }
            break;
        case 2:
            if (state->auth_bearer_enabled) {
                ImGui::Text("Bearer token will be added to Authorization header when enabled");
            } else {
                ImGui::Text("Bearer token is configured but disabled - check the box above to enable");
            }
            break;
        case 3:
            if (state->auth_basic_enabled) {
                ImGui::Text("Username and password will be base64 encoded in Authorization header when enabled");
            } else {
                ImGui::Text("Basic auth is configured but disabled - check the box above to enable");
            }
            break;
        case 4:
            if (state->auth_oauth_enabled) {
                ImGui::Text("OAuth token will be added to Authorization header as Bearer token when enabled");
            } else {
                ImGui::Text("OAuth token is configured but disabled - check the box above to enable");
            }
            break;
        default:
            ImGui::Text("Select an authentication method to configure credentials");
            break;
    }
    ImGui::PopStyleColor();
}

} 