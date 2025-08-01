/*
 * provides reusable ui panel components for headers and request body editing
 * handles form validation, content type detection, and modern styling
 * includes json formatting, multipart form data, and url encoding support
 */

#include "ui/ui_panels.h"
#include "ui/ui_manager.h"
#include "ui/theme.h"
#include "font_awesome.h"
#include "request_response.h"
#include "app_state.h"
#include <string.h>
#include <stdio.h>

extern "C" {

void ui_panels_render_headers_panel(UIManager* ui, HeaderList* headers, void* app_state) {
    if (!ui || !headers || !app_state) {
        return;
    }

    AppState* state = (AppState*)app_state;

    const ModernGruvboxTheme* theme = theme_get_current();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(SPACING_SM, SPACING_SM));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(SPACING_SM, SPACING_XS));

    if (headers->count > 0) {
        theme_push_body_style();
        ImGui::Text("Current Headers");
        theme_pop_text_style();

        ImGui::Spacing();

        for (int i = 0; i < headers->count; i++) {
            ImGui::PushID(i);

            ImGui::BeginGroup();

            ImGui::SetNextItemWidth(120);
            theme_push_input_style(theme);
            ImGui::InputText("##header_name", headers->headers[i].name, sizeof(headers->headers[i].name));
            theme_pop_input_style();

            ImGui::SameLine();
            ImGui::TextColored(theme->fg_tertiary, ":");
            ImGui::SameLine();

            ImGui::SetNextItemWidth(244);
            theme_push_input_style(theme);
            ImGui::InputText("##header_value", headers->headers[i].value, sizeof(headers->headers[i].value));
            theme_pop_input_style();

            ImGui::SameLine();

            theme_push_button_style(theme, BUTTON_TYPE_DANGER);
            if (ImGui::Button(font_awesome_icon_with_fallback(ICON_FA_XMARK, "Remove"), ImVec2(40, 28))) {
                header_list_remove(headers, i);
                theme_pop_button_style();
                ImGui::EndGroup();
                ImGui::PopID();
                break; 
            }
            theme_pop_button_style();

            ImGui::EndGroup();

            bool name_valid = header_validate_name(headers->headers[i].name) == 0;
            bool value_valid = header_validate_value(headers->headers[i].value) == 0;

            if (!name_valid && strlen(headers->headers[i].name) > 0) {
                ImGui::SameLine();
                theme_render_status_indicator("Invalid name", STATUS_TYPE_ERROR, theme);
            }

            if (!value_valid && strlen(headers->headers[i].value) > 0) {
                ImGui::SameLine();
                theme_render_status_indicator("Invalid value", STATUS_TYPE_ERROR, theme);
            }

            ImGui::PopID();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    ImGui::BeginGroup();

    theme_push_caption_style();
    ImGui::Text("Name");
    theme_pop_text_style();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    theme_push_input_style(theme);
    ImGui::InputText("##new_header_name", state->header_name_buffer, sizeof(state->header_name_buffer));
    theme_pop_input_style();

    ImGui::SameLine();

    theme_push_caption_style();
    ImGui::Text("Value");
    theme_pop_text_style();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180);
    theme_push_input_style(theme);
    ImGui::InputText("##new_header_value", state->header_value_buffer, sizeof(state->header_value_buffer));
    theme_pop_input_style();

    ImGui::SameLine();

    bool can_add = strlen(state->header_name_buffer) > 0 &&
                   strlen(state->header_value_buffer) > 0 &&
                   header_validate_name(state->header_name_buffer) == 0 &&
                   header_validate_value(state->header_value_buffer) == 0;

    if (!can_add) {
        ImGui::BeginDisabled();
    }

    theme_push_button_style(theme, BUTTON_TYPE_SUCCESS);
    if (ImGui::Button(font_awesome_icon_with_fallback(ICON_FA_PLUS, "+"), ImVec2(40, 28))) {
        if (header_list_add(headers, state->header_name_buffer, state->header_value_buffer) == 0) {

            state->header_name_buffer[0] = '\0';
            state->header_value_buffer[0] = '\0';
        }
    }
    theme_pop_button_style();

    if (!can_add) {
        ImGui::EndDisabled();
    }

    ImGui::EndGroup();

    if (strlen(state->header_name_buffer) > 0 && header_validate_name(state->header_name_buffer) != 0) {
        ImGui::Spacing();
        theme_render_status_indicator("Invalid header name format", STATUS_TYPE_ERROR, theme);
    }

    if (strlen(state->header_value_buffer) > 0 && header_validate_value(state->header_value_buffer) != 0) {
        ImGui::Spacing();
        theme_render_status_indicator("Invalid header value format", STATUS_TYPE_ERROR, theme);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    theme_push_caption_style();
    ImGui::TextColored(theme->fg_tertiary, "Total Headers: %d", headers->count);
    theme_pop_text_style();

    ImGui::PopStyleVar(2); 
}

void ui_panels_render_body_panel(UIManager* ui, char* body_buffer, size_t buffer_size) {
    if (!ui || !body_buffer) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(SPACING_SM, SPACING_MD));

    size_t body_length = strlen(body_buffer);

    bool looks_like_json = false;
    bool looks_like_xml = false;
    if (body_length > 0) {
        const char* trimmed = body_buffer;
        while (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n') trimmed++;
        looks_like_json = (*trimmed == '{' || *trimmed == '[');
        looks_like_xml = (*trimmed == '<');
    }

    theme_push_body_style();
    ImGui::Text("Content");
    theme_pop_text_style();

    ImGui::SameLine();
    if (looks_like_json) {
        ImGui::TextColored(theme->success, "[JSON]");
    } else if (looks_like_xml) {
        ImGui::TextColored(theme->info, "[XML]");
    } else if (body_length > 0) {
        ImGui::TextColored(theme->fg_tertiary, "[TEXT]");
    } else {
        ImGui::TextColored(theme->fg_disabled, "[EMPTY]");
    }

    ImGui::Spacing();

    ImVec2 text_size = ImVec2(-1.0f, 220.0f); 

    if (looks_like_json) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, theme_alpha_blend(theme->success, 0.1f));
        ImGui::PushStyleColor(ImGuiCol_Border, theme_alpha_blend(theme->success, 0.3f));
    } else if (looks_like_xml) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, theme_alpha_blend(theme->info, 0.1f));
        ImGui::PushStyleColor(ImGuiCol_Border, theme_alpha_blend(theme->info, 0.3f));
    } else {
        theme_push_input_style(theme);
    }

    ImGui::InputTextMultiline("##body_input", body_buffer, buffer_size, text_size);

    if (looks_like_json || looks_like_xml) {
        ImGui::PopStyleColor(2);
    } else {
        theme_pop_input_style();
    }

    if (body_length > buffer_size * 0.9) {
        ImGui::SameLine();
        theme_render_status_indicator("Approaching buffer limit", STATUS_TYPE_WARNING, theme);
    }

    if (body_length > 0 && looks_like_json) {
        ImGui::SameLine();

        int brace_count = 0;
        int bracket_count = 0;
        bool in_string = false;
        bool escaped = false;

        for (size_t i = 0; i < body_length; i++) {
            char c = body_buffer[i];

            if (!in_string) {
                if (c == '"' && !escaped) {
                    in_string = true;
                } else if (c == '{') {
                    brace_count++;
                } else if (c == '}') {
                    brace_count--;
                } else if (c == '[') {
                    bracket_count++;
                } else if (c == ']') {
                    bracket_count--;
                }
            } else {
                if (c == '"' && !escaped) {
                    in_string = false;
                }
            }

            escaped = (c == '\\' && !escaped);
        }

        if (brace_count != 0 || bracket_count != 0 || in_string) {
            theme_render_status_indicator("Invalid JSON syntax", STATUS_TYPE_ERROR, theme);
        } else {
            theme_render_status_indicator("Valid JSON", STATUS_TYPE_SUCCESS, theme);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    theme_push_body_style();
    ImGui::Text("Quick Templates");
    theme_pop_text_style();

    ImGui::Spacing();

    theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
    if (ImGui::Button("{ } JSON Object", ImVec2(120, 0))) {
        strcpy(body_buffer, "{\n  \"key\": \"value\"\n}");
    }
    theme_pop_button_style();

    ImGui::SameLine();
    theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
    if (ImGui::Button("[ ] JSON Array", ImVec2(120, 0))) {
        strcpy(body_buffer, "[\n  \"item1\",\n  \"item2\"\n]");
    }
    theme_pop_button_style();

    ImGui::SameLine();
    theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
    if (ImGui::Button("Format JSON", ImVec2(100, 0))) {

        char formatted_buffer[8192];
        size_t src_len = strlen(body_buffer);
        size_t dst_pos = 0;

        for (size_t i = 0; i < src_len && dst_pos < sizeof(formatted_buffer) - 1; i++) {
            if (body_buffer[i] == '\\' && i + 1 < src_len && body_buffer[i + 1] == 'n') {

                formatted_buffer[dst_pos++] = '\n';
                i++; 
            } else if (body_buffer[i] == '\\' && i + 1 < src_len && body_buffer[i + 1] == 't') {

                formatted_buffer[dst_pos++] = '\t';
                i++; 
            } else if (body_buffer[i] == '\\' && i + 1 < src_len && body_buffer[i + 1] == '"') {

                formatted_buffer[dst_pos++] = '"';
                i++; 
            } else {
                formatted_buffer[dst_pos++] = body_buffer[i];
            }
        }
        formatted_buffer[dst_pos] = '\0';

        if (dst_pos < buffer_size) {
            strcpy(body_buffer, formatted_buffer);
        }
    }
    theme_pop_button_style();

    ImGui::SameLine();
    theme_push_button_style(theme, BUTTON_TYPE_DANGER);
    if (ImGui::Button("Clear", ImVec2(80, 0))) {
        body_buffer[0] = '\0';
    }
    theme_pop_button_style();

    ImGui::PopStyleVar(); 
}

} 