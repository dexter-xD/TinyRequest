#include "ui/ui_response_panel.h"
#include "ui/ui_manager.h"
#include "ui/theme.h"
#include "font_awesome.h"
#include "app_state.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cjson/cJSON.h>

extern "C" {

static void render_json_with_simple_coloring(const char* json_text, const ModernGruvboxTheme* theme) {
    if (!json_text || !theme) {
        return;
    }

    ImVec4 json_color = theme->success;
    json_color.w = 0.9f; 

    ImGui::TextColored(json_color, "%s", json_text);
}

static char* format_json_string(const char* json_string) {
    if (!json_string || strlen(json_string) == 0) {
        return NULL;
    }

    cJSON* json = cJSON_Parse(json_string);
    if (!json) {

        char* result = (char*)malloc(strlen(json_string) + 1);
        if (result) {
            strcpy(result, json_string);
        }
        return result;
    }

    char* formatted = cJSON_Print(json);
    cJSON_Delete(json);

    return formatted;
}

static char* g_formatted_json = NULL;
static bool g_show_formatted = true; 
static bool g_word_wrap_enabled = true; 

static void cleanup_formatted_json(void) {
    if (g_formatted_json) {
        free(g_formatted_json);
        g_formatted_json = NULL;
    }
    g_show_formatted = true; 

}

static void __attribute__((destructor)) ui_response_panel_destructor(void) {
    cleanup_formatted_json();
}

static void reset_json_formatting_state(void) {
    cleanup_formatted_json();
}

static void auto_format_json_if_needed(const char* json_string) {
    if (!json_string || strlen(json_string) == 0) {
        return;
    }

    if (g_formatted_json) {
        free(g_formatted_json);
        g_formatted_json = NULL;
    }

    g_formatted_json = format_json_string(json_string);
}

void ui_response_panel_render(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();
    Response* response = &state->current_response;

    static int last_status_code = -1;
    static size_t last_body_size = 0;
    static char* last_body_ptr = NULL;

    if (response->status_code != last_status_code ||
        response->body_size != last_body_size ||
        response->body != last_body_ptr) {
        reset_json_formatting_state();
        last_status_code = response->status_code;
        last_body_size = response->body_size;
        last_body_ptr = response->body;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(SPACING_SM, SPACING_MD));

    if (response->status_code > 0) {

        ImGui::BeginGroup();

        ImVec4 status_color;
        char status_display[32];

        if (response->status_code >= 200 && response->status_code < 300) {
            status_color = theme->success;
        } else if (response->status_code >= 300 && response->status_code < 400) {
            status_color = theme->warning;
        } else if (response->status_code >= 400 && response->status_code < 500) {
            status_color = theme->warning;
        } else if (response->status_code >= 500) {
            status_color = theme->error;
        } else {
            status_color = theme->fg_tertiary;
        }

        const char* status_text_to_use = response->status_text;

        if (strlen(response->status_text) == 0 || strcmp(response->status_text, "OK") == 0) {
            switch (response->status_code) {
                case 200: status_text_to_use = "OK"; break;
                case 201: status_text_to_use = "Created"; break;
                case 202: status_text_to_use = "Accepted"; break;
                case 204: status_text_to_use = "No Content"; break;
                case 301: status_text_to_use = "Moved Permanently"; break;
                case 302: status_text_to_use = "Found"; break;
                case 304: status_text_to_use = "Not Modified"; break;
                case 400: status_text_to_use = "Bad Request"; break;
                case 401: status_text_to_use = "Unauthorized"; break;
                case 403: status_text_to_use = "Forbidden"; break;
                case 404: status_text_to_use = "Not Found"; break;
                case 405: status_text_to_use = "Method Not Allowed"; break;
                case 409: status_text_to_use = "Conflict"; break;
                case 422: status_text_to_use = "Unprocessable Entity"; break;
                case 429: status_text_to_use = "Too Many Requests"; break;
                case 500: status_text_to_use = "Internal Server Error"; break;
                case 502: status_text_to_use = "Bad Gateway"; break;
                case 503: status_text_to_use = "Service Unavailable"; break;
                case 504: status_text_to_use = "Gateway Timeout"; break;
                default:
                    if (strlen(response->status_text) > 0) {
                        status_text_to_use = response->status_text;
                    } else {
                        status_text_to_use = "Unknown";
                    }
                    break;
            }
        }

        snprintf(status_display, sizeof(status_display), "%d %s", response->status_code, status_text_to_use);

        ImGui::PushStyleColor(ImGuiCol_Button, status_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, status_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, status_color);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); 
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f); 
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f)); 
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f)); 

        ImVec2 text_size = ImGui::CalcTextSize(status_display);
        float badge_width = (text_size.x + 16.0f > 60.0f) ? text_size.x + 16.0f : 60.0f; 

        ImGui::Button(status_display, ImVec2(badge_width, 25));
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(4);

        ImGui::SameLine();

        ImVec4 time_color = theme->fg_secondary;
        if (response->response_time > 0) {
            if (response->response_time < 100) {
                time_color = theme->success;
            } else if (response->response_time < 1000) {
                time_color = theme->warning;
            } else {
                time_color = theme->error;
            }
        }

        char time_text[32];
        if (response->response_time > 0) {
            snprintf(time_text, sizeof(time_text), "%.0f ms", response->response_time);
        } else {
            strcpy(time_text, "-- ms");
        }

        ImGui::PushStyleColor(ImGuiCol_Button, theme_alpha_blend(time_color, 0.2f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme_alpha_blend(time_color, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme_alpha_blend(time_color, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_Text, time_color);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f); 
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f)); 
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f)); 
        ImGui::Button(time_text, ImVec2(70, 25));
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(4);

        ImGui::SameLine();

        char size_text[32];
        if (response->body_size > 0) {
            if (response->body_size < 1024) {
                snprintf(size_text, sizeof(size_text), "%zu B", response->body_size);
            } else if (response->body_size < 1024 * 1024) {
                snprintf(size_text, sizeof(size_text), "%.1f KB", response->body_size / 1024.0);
            } else {
                snprintf(size_text, sizeof(size_text), "%.1f MB", response->body_size / (1024.0 * 1024.0));
            }
        } else {
            strcpy(size_text, "0 B");
        }

        ImGui::PushStyleColor(ImGuiCol_Button, theme_alpha_blend(theme->info, 0.2f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme_alpha_blend(theme->info, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme_alpha_blend(theme->info, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_Text, theme->info);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f); 
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f)); 
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f)); 
        ImGui::Button(size_text, ImVec2(70, 25));
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(4);

        ImGui::EndGroup();

        ImGui::Separator();

        static int selected_tab = 0;
        const char* tab_names[] = { "Preview", "Headers", "Cookies" };
        const int tab_count = sizeof(tab_names) / sizeof(tab_names[0]);

        ImGui::BeginGroup();
        for (int i = 0; i < tab_count; i++) {
            if (i > 0) ImGui::SameLine();

            bool is_selected = (selected_tab == i);

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
            if (i == 1) { 
                snprintf(tab_label, sizeof(tab_label), "%s %d", tab_names[i], response->headers.count);
            } else {
                strcpy(tab_label, tab_names[i]);
            }

            if (ImGui::Button(tab_label, ImVec2(90, 30))) {
                selected_tab = i;
            }

            ImGui::PopStyleColor(4);
        }
        ImGui::EndGroup();

        ImGui::Separator();

        if (selected_tab == 0) {
            if (response->body && response->body_size > 0) {

                bool is_json = false;
                bool is_xml = false;
                bool is_html = false;
                const char* content_type = NULL;

                for (int i = 0; i < response->headers.count; i++) {
                    if (strcasecmp(response->headers.headers[i].name, "content-type") == 0) {
                        content_type = response->headers.headers[i].value;
                        break;
                    }
                }

                if (content_type) {
                    is_json = strstr(content_type, "application/json") != NULL;
                    is_xml = strstr(content_type, "application/xml") != NULL || strstr(content_type, "text/xml") != NULL;
                    is_html = strstr(content_type, "text/html") != NULL;
                } else if (response->body) {

                    const char* trimmed = response->body;
                    while (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n') trimmed++;
                    is_json = (*trimmed == '{' || *trimmed == '[');
                    is_xml = (*trimmed == '<' && strncmp(trimmed, "<?xml", 5) == 0);
                    is_html = (*trimmed == '<' && (strncmp(trimmed, "<!DOCTYPE", 9) == 0 || strncmp(trimmed, "<html", 5) == 0));
                }

                if (is_json && !g_formatted_json) {
                    auto_format_json_if_needed(response->body);
                }

                const size_t MAX_DISPLAY_SIZE = 100000; 
                bool is_truncated = response->body_size > MAX_DISPLAY_SIZE;

                if (is_truncated) {
                    theme_render_status_indicator(
                        "Large response truncated for performance",
                        STATUS_TYPE_WARNING,
                        theme
                    );
                    ImGui::Spacing();
                }

                ImVec2 body_size = ImVec2(-1.0f, -60.0f); 

                if (is_json) {
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme_alpha_blend(theme->success, 0.1f));
                    ImGui::PushStyleColor(ImGuiCol_Border, theme_alpha_blend(theme->success, 0.3f));
                    ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_primary);
                } else if (is_xml) {
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme_alpha_blend(theme->warning, 0.1f));
                    ImGui::PushStyleColor(ImGuiCol_Border, theme_alpha_blend(theme->warning, 0.3f));
                    ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_primary);
                } else if (is_html) {
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme_alpha_blend(theme->accent_primary, 0.1f));
                    ImGui::PushStyleColor(ImGuiCol_Border, theme_alpha_blend(theme->accent_primary, 0.3f));
                    ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_primary);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->bg_input);
                    ImGui::PushStyleColor(ImGuiCol_Border, theme->border_normal);
                    ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_primary);
                }

                if (g_word_wrap_enabled) {

                    ImGui::BeginChild("ResponseBodyWrap", body_size, true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

                    ImGui::PushTextWrapPos(0.0f);

                    if (is_json && g_show_formatted && g_formatted_json) {

                        if (strlen(g_formatted_json) > MAX_DISPLAY_SIZE) {
                            char temp_buffer[MAX_DISPLAY_SIZE + 1];
                            memcpy(temp_buffer, g_formatted_json, MAX_DISPLAY_SIZE);
                            temp_buffer[MAX_DISPLAY_SIZE] = '\0';
                            render_json_with_simple_coloring(temp_buffer, theme);
                        } else {
                            render_json_with_simple_coloring(g_formatted_json, theme);
                        }
                    } else if (is_json) {

                        if (is_truncated) {
                            char temp_buffer[MAX_DISPLAY_SIZE + 1];
                            memcpy(temp_buffer, response->body, MAX_DISPLAY_SIZE);
                            temp_buffer[MAX_DISPLAY_SIZE] = '\0';
                            render_json_with_simple_coloring(temp_buffer, theme);
                        } else {
                            render_json_with_simple_coloring(response->body, theme);
                        }
                    } else {

                        if (is_truncated) {
                            char temp_buffer[MAX_DISPLAY_SIZE + 1];
                            memcpy(temp_buffer, response->body, MAX_DISPLAY_SIZE);
                            temp_buffer[MAX_DISPLAY_SIZE] = '\0';
                            ImGui::TextUnformatted(temp_buffer);
                        } else {
                            ImGui::TextUnformatted(response->body);
                        }
                    }

                    ImGui::PopTextWrapPos();
                    ImGui::EndChild();
                } else {

                    ImGui::BeginChild("ResponseBodyScroll", body_size, true,
                                     ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

                    if (is_json && g_show_formatted && g_formatted_json) {

                        if (strlen(g_formatted_json) > MAX_DISPLAY_SIZE) {
                            char temp_buffer[MAX_DISPLAY_SIZE + 1];
                            memcpy(temp_buffer, g_formatted_json, MAX_DISPLAY_SIZE);
                            temp_buffer[MAX_DISPLAY_SIZE] = '\0';
                            render_json_with_simple_coloring(temp_buffer, theme);
                        } else {
                            render_json_with_simple_coloring(g_formatted_json, theme);
                        }
                    } else if (is_json) {

                        if (is_truncated) {
                            char temp_buffer[MAX_DISPLAY_SIZE + 1];
                            memcpy(temp_buffer, response->body, MAX_DISPLAY_SIZE);
                            temp_buffer[MAX_DISPLAY_SIZE] = '\0';
                            render_json_with_simple_coloring(temp_buffer, theme);
                        } else {
                            render_json_with_simple_coloring(response->body, theme);
                        }
                    } else {

                        if (is_truncated) {
                            char temp_buffer[MAX_DISPLAY_SIZE + 1];
                            memcpy(temp_buffer, response->body, MAX_DISPLAY_SIZE);
                            temp_buffer[MAX_DISPLAY_SIZE] = '\0';
                            ImGui::TextUnformatted(temp_buffer);
                        } else {
                            ImGui::TextUnformatted(response->body);
                        }
                    }

                    ImGui::EndChild();
                }

                ImGui::PopStyleColor(3);

                ImGui::Spacing();

                theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
                if (ImGui::Button("Copy", ImVec2(80, 0))) {

                    if (is_json && g_show_formatted && g_formatted_json) {
                        ImGui::SetClipboardText(g_formatted_json);
                    } else {
                        ImGui::SetClipboardText(response->body);
                    }
                }
                theme_pop_button_style();

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(g_show_formatted ? "Copy formatted JSON" : "Copy response body");
                }

                ImGui::SameLine();
                theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
                if (ImGui::Button(g_word_wrap_enabled ? ICON_FA_LIST " Wrap" : ICON_FA_ARROW_RIGHT " No Wrap", ImVec2(90, 0))) {
                    g_word_wrap_enabled = !g_word_wrap_enabled;
                }
                theme_pop_button_style();

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(g_word_wrap_enabled ? "Disable word wrap (show horizontal scrollbar)" : "Enable word wrap (wrap long lines)");
                }

                if (is_json) {
                    ImGui::SameLine();
                    theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
                    if (ImGui::Button(g_show_formatted ? "Raw" : "Format", ImVec2(80, 0))) {
                        if (!g_show_formatted) {

                            if (g_formatted_json) {
                                free(g_formatted_json);
                                g_formatted_json = NULL;
                            }

                            g_formatted_json = format_json_string(response->body);
                            if (g_formatted_json) {
                                g_show_formatted = true;
                            } else {

                                g_show_formatted = false;
                            }
                        } else {

                            g_show_formatted = false;
                        }
                    }
                    theme_pop_button_style();

                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip(g_show_formatted ? "Show raw JSON" : "Format JSON with indentation");
                    }
                }

            } else {
                theme_render_status_indicator("No response body received", STATUS_TYPE_INFO, theme);
            }
        }

        else if (selected_tab == 1) {
            if (response->headers.count > 0) {

                ImVec2 headers_size = ImVec2(-1.0f, -60.0f); 

                ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->bg_input);
                ImGui::PushStyleColor(ImGuiCol_Border, theme->border_normal);
                ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_primary);

                static bool headers_word_wrap_enabled = true;

                if (headers_word_wrap_enabled) {

                    ImGui::BeginChild("ResponseHeadersWrap", headers_size, true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(SPACING_SM, 2.0f));

                    ImGui::PushTextWrapPos(0.0f);

                    for (int i = 0; i < response->headers.count; i++) {

                        char header_line[1024];
                        snprintf(header_line, sizeof(header_line), "%s: %s",
                                response->headers.headers[i].name,
                                response->headers.headers[i].value);

                        ImGui::TextColored(theme->accent_secondary, "%s:", response->headers.headers[i].name);
                        ImGui::SameLine();
                        ImGui::TextUnformatted(response->headers.headers[i].value);

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Right-click to copy header");
                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                                ImGui::SetClipboardText(header_line);
                            }
                        }
                    }

                    ImGui::PopTextWrapPos();
                    ImGui::PopStyleVar(); 
                    ImGui::EndChild();
                } else {

                    ImGui::BeginChild("ResponseHeadersScroll", headers_size, true,
                                     ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(SPACING_SM, 2.0f));

                    for (int i = 0; i < response->headers.count; i++) {

                        char header_line[1024];
                        snprintf(header_line, sizeof(header_line), "%s: %s",
                                response->headers.headers[i].name,
                                response->headers.headers[i].value);

                        ImGui::TextColored(theme->accent_secondary, "%s:", response->headers.headers[i].name);
                        ImGui::SameLine();
                        ImGui::TextUnformatted(response->headers.headers[i].value);

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Right-click to copy header");
                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                                ImGui::SetClipboardText(header_line);
                            }
                        }
                    }

                    ImGui::PopStyleVar(); 
                    ImGui::EndChild();
                }

                ImGui::PopStyleColor(3);

                ImGui::Spacing();

                theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
                if (ImGui::Button("Copy All", ImVec2(80, 0))) {

                    char all_headers[4096] = {0};
                    for (int i = 0; i < response->headers.count; i++) {
                        char header_line[512];
                        snprintf(header_line, sizeof(header_line), "%s: %s\n",
                                response->headers.headers[i].name,
                                response->headers.headers[i].value);
                        strncat(all_headers, header_line, sizeof(all_headers) - strlen(all_headers) - 1);
                    }
                    ImGui::SetClipboardText(all_headers);
                }
                theme_pop_button_style();

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Copy all headers to clipboard");
                }

                ImGui::SameLine();
                theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
                if (ImGui::Button(headers_word_wrap_enabled ? ICON_FA_LIST " Wrap" : ICON_FA_ARROW_RIGHT " No Wrap", ImVec2(90, 0))) {
                    headers_word_wrap_enabled = !headers_word_wrap_enabled;
                }
                theme_pop_button_style();

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(headers_word_wrap_enabled ? "Disable word wrap (show horizontal scrollbar)" : "Enable word wrap (wrap long lines)");
                }

            } else {
                theme_render_status_indicator("No response headers received", STATUS_TYPE_INFO, theme);
            }
        }

        else if (selected_tab == 2) {

            static char parsed_cookies[4096] = {0};
            static int cookie_count = 0;
            static bool cookies_parsed = false;
            static int last_response_status = -1;
            static char* last_response_body = NULL;

            if (response->status_code != last_response_status || response->body != last_response_body) {
                cookies_parsed = false;
                last_response_status = response->status_code;
                last_response_body = response->body;
            }

            if (!cookies_parsed) {
                parsed_cookies[0] = '\0';
                cookie_count = 0;

                for (int i = 0; i < response->headers.count; i++) {
                    if (strcasecmp(response->headers.headers[i].name, "set-cookie") == 0) {
                        const char* cookie_value = response->headers.headers[i].value;

                        char cookie_name[128] = {0};
                        char cookie_val[512] = {0};
                        char cookie_domain[256] = {0};
                        char cookie_path[256] = {0};
                        char cookie_expires[128] = {0};
                        bool secure = false;
                        bool httponly = false;
                        bool samesite = false;

                        const char* semicolon = strchr(cookie_value, ';');
                        const char* equals = strchr(cookie_value, '=');

                        if (equals && (semicolon == NULL || equals < semicolon)) {
                            size_t name_len = equals - cookie_value;
                            size_t value_len = semicolon ? (semicolon - equals - 1) : strlen(equals + 1);

                            if (name_len < sizeof(cookie_name) && value_len < sizeof(cookie_val)) {
                                strncpy(cookie_name, cookie_value, name_len);
                                cookie_name[name_len] = '\0';
                                strncpy(cookie_val, equals + 1, value_len);
                                cookie_val[value_len] = '\0';
                            }
                        }

                        if (semicolon) {
                            const char* attr_start = semicolon + 1;
                            while (*attr_start) {

                                while (*attr_start == ' ' || *attr_start == '\t') attr_start++;
                                if (!*attr_start) break;

                                const char* attr_end = strchr(attr_start, ';');
                                if (!attr_end) attr_end = attr_start + strlen(attr_start);

                                if (strncasecmp(attr_start, "domain=", 7) == 0) {
                                    size_t len = attr_end - attr_start - 7;
                                    if (len < sizeof(cookie_domain)) {
                                        strncpy(cookie_domain, attr_start + 7, len);
                                        cookie_domain[len] = '\0';
                                    }
                                } else if (strncasecmp(attr_start, "path=", 5) == 0) {
                                    size_t len = attr_end - attr_start - 5;
                                    if (len < sizeof(cookie_path)) {
                                        strncpy(cookie_path, attr_start + 5, len);
                                        cookie_path[len] = '\0';
                                    }
                                } else if (strncasecmp(attr_start, "expires=", 8) == 0) {
                                    size_t len = attr_end - attr_start - 8;
                                    if (len < sizeof(cookie_expires)) {
                                        strncpy(cookie_expires, attr_start + 8, len);
                                        cookie_expires[len] = '\0';
                                    }
                                } else if (strncasecmp(attr_start, "secure", 6) == 0) {
                                    secure = true;
                                } else if (strncasecmp(attr_start, "httponly", 8) == 0) {
                                    httponly = true;
                                } else if (strncasecmp(attr_start, "samesite", 8) == 0) {
                                    samesite = true;
                                }

                                attr_start = (*attr_end == ';') ? attr_end + 1 : attr_end;
                            }
                        }

                        if (strlen(cookie_name) > 0) {
                            char cookie_line[1024];
                            snprintf(cookie_line, sizeof(cookie_line),
                                    "Name: %s\nValue: %s\nDomain: %s\nPath: %s\nExpires: %s\nSecure: %s\nHttpOnly: %s\nSameSite: %s\n\n",
                                    cookie_name,
                                    strlen(cookie_val) > 0 ? cookie_val : "(empty)",
                                    strlen(cookie_domain) > 0 ? cookie_domain : "(not set)",
                                    strlen(cookie_path) > 0 ? cookie_path : "(not set)",
                                    strlen(cookie_expires) > 0 ? cookie_expires : "(session)",
                                    secure ? "Yes" : "No",
                                    httponly ? "Yes" : "No",
                                    samesite ? "Yes" : "No");

                            size_t current_len = strlen(parsed_cookies);
                            size_t line_len = strlen(cookie_line);
                            if (current_len + line_len < sizeof(parsed_cookies) - 1) {
                                strcat(parsed_cookies, cookie_line);
                                cookie_count++;
                            }
                        }
                    }
                }
                cookies_parsed = true;
            }

            if (cookie_count > 0) {

                ImVec2 cookies_size = ImVec2(-1.0f, -60.0f); 

                ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->bg_input);
                ImGui::PushStyleColor(ImGuiCol_Border, theme->border_normal);
                ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_primary);

                static bool cookies_word_wrap_enabled = true;

                if (cookies_word_wrap_enabled) {

                    ImGui::BeginChild("ResponseCookiesWrap", cookies_size, true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(SPACING_SM, 2.0f));

                    ImGui::PushTextWrapPos(0.0f);

                    ImGui::TextUnformatted(parsed_cookies);

                    ImGui::PopTextWrapPos();
                    ImGui::PopStyleVar(); 
                    ImGui::EndChild();
                } else {

                    ImGui::BeginChild("ResponseCookiesScroll", cookies_size, true,
                                     ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(SPACING_SM, 2.0f));

                    ImGui::TextUnformatted(parsed_cookies);

                    ImGui::PopStyleVar(); 
                    ImGui::EndChild();
                }

                ImGui::PopStyleColor(3);

                ImGui::Spacing();

                theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
                if (ImGui::Button("Copy All", ImVec2(80, 0))) {
                    ImGui::SetClipboardText(parsed_cookies);
                }
                theme_pop_button_style();

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Copy all cookies to clipboard");
                }

                ImGui::SameLine();
                theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
                if (ImGui::Button(cookies_word_wrap_enabled ? ICON_FA_LIST " Wrap" : ICON_FA_ARROW_RIGHT " No Wrap", ImVec2(90, 0))) {
                    cookies_word_wrap_enabled = !cookies_word_wrap_enabled;
                }
                theme_pop_button_style();

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(cookies_word_wrap_enabled ? "Disable word wrap (show horizontal scrollbar)" : "Enable word wrap (wrap long lines)");
                }

                ImGui::SameLine();
                theme_push_button_style(theme, BUTTON_TYPE_PRIMARY);
                if (ImGui::Button(ICON_FA_COG " Manage Cookies", ImVec2(140, 0))) {

                    state->show_cookie_manager = true;
                }
                theme_pop_button_style();

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Open cookie management dialog");
                }

            } else {
                theme_render_status_indicator("No cookies found in response", STATUS_TYPE_INFO, theme);

                ImGui::Spacing();
                theme_push_button_style(theme, BUTTON_TYPE_PRIMARY);
                if (ImGui::Button(ICON_FA_COG " Manage Cookies", ImVec2(140, 0))) {

                    state->show_cookie_manager = true;
                }
                theme_pop_button_style();

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Open cookie management dialog");
                }
            }
        }

        else if (selected_tab == 3) {
            theme_render_status_indicator("Test functionality not implemented yet", STATUS_TYPE_INFO, theme);
        }

    } else {

        ImGui::Spacing();
        ImGui::Spacing();

        ImVec2 window_size = ImGui::GetWindowSize();
        ImVec2 text_size = ImGui::CalcTextSize("No response received yet");
        ImGui::SetCursorPosX((window_size.x - text_size.x) * 0.5f);

        theme_render_status_indicator("No response received yet", STATUS_TYPE_INFO, theme);

        ImGui::Spacing();

        ImVec2 help_size = ImGui::CalcTextSize("Configure your request and click 'Send Request'");
        ImGui::SetCursorPosX((window_size.x - help_size.x) * 0.5f);

        theme_push_caption_style();
        ImGui::TextColored(theme->fg_tertiary, "Configure your request and click 'Send Request'");
        theme_pop_text_style();

        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::Separator();
        ImGui::Spacing();

        theme_push_body_style();
        ImGui::Text("[TIPS] Quick Tips");
        theme_pop_text_style();

        ImGui::Spacing();

        theme_push_caption_style();
        ImGui::BulletText("Use Ctrl+R to quickly send requests");
        ImGui::BulletText("Save frequently used requests with Ctrl+S");
        ImGui::BulletText("Load saved requests with Ctrl+O");
        ImGui::BulletText("Right-click response headers to copy them");
        theme_pop_text_style();
    }

    ImGui::PopStyleVar(); 
}

void ui_response_panel_cleanup(void) {
    cleanup_formatted_json();
}

} 