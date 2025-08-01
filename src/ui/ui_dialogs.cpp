#include "ui/ui_dialogs.h"
#include "ui/ui_manager.h"
#include "ui/ui_core.h"
#include "ui/theme.h"
#include "font_awesome.h"
#include "app_state.h"
#include "persistence.h"
#include <string.h>
#include <stdio.h>
#include <cstdlib>

extern "C" {

void ui_dialogs_render_save_dialog(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(280, 320), ImGuiCond_Appearing);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(SPACING_LG, SPACING_LG));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(SPACING_MD, SPACING_MD));

    if (ImGui::BeginPopupModal("Save Request", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {

        theme_push_header_style();
        font_awesome_render_icon_text(ICON_FA_SAVE, ICON_FALLBACK_SAVE, "Save Request Configuration", theme->accent_primary);
        theme_pop_text_style();

        ImGui::Spacing();

        theme_push_body_style();
        ImGui::Text("Request Name");
        theme_pop_text_style();

        ImGui::SetNextItemWidth(-1.0f); 
        theme_push_input_style(theme);
        ImGui::InputText("##save_name", state->save_request_name, sizeof(state->save_request_name));
        theme_pop_input_style();

        ImGui::Spacing();

        theme_push_body_style();
        font_awesome_render_icon_text(ICON_FA_INFO, ICON_FALLBACK_SAVE, "Request Summary", theme->accent_primary);
        theme_pop_text_style();

        theme_push_caption_style();
        ImGui::TextColored(theme->accent_secondary, "[METHOD]:");
        ImGui::SameLine();
        ImGui::TextColored(theme->fg_secondary, "%s", ui_core_get_method_string(state->selected_method_index));

        ImGui::TextColored(theme->accent_secondary, "[URL]:");
        ImGui::SameLine();
        ImGui::TextColored(theme->fg_secondary, "%s", state->url_buffer);

        ImGui::TextColored(theme->accent_secondary, "[HEADERS]:");
        ImGui::SameLine();
        ImGui::TextColored(theme->fg_secondary, "%d", state->current_request.headers.count);

        size_t body_len = strlen(state->body_buffer);
        ImGui::TextColored(theme->accent_secondary, "[BODY]:");
        ImGui::SameLine();
        if (body_len > 0) {
            if (body_len < 1024) {
                ImGui::TextColored(theme->fg_secondary, "%zu bytes", body_len);
            } else {
                ImGui::TextColored(theme->fg_secondary, "%.1f KB", body_len / 1024.0f);
            }
        } else {
            ImGui::TextColored(theme->fg_disabled, "(empty)");
        }
        theme_pop_text_style();

        ImGui::Spacing();

        if (state->save_error_message[0] != '\0') {
            ImGui::Separator();
            ImGui::Spacing();
            theme_render_status_indicator(state->save_error_message, STATUS_TYPE_ERROR, theme);
            ImGui::Spacing();
        }

        ImGui::Spacing();

        bool can_save = strlen(state->save_request_name) > 0;

        if (!can_save) {
            ImGui::BeginDisabled();
        }

        theme_push_button_style(theme, BUTTON_TYPE_SUCCESS);
        if (ImGui::Button(font_awesome_icon_with_fallback(ICON_FA_SAVE, "Save"), ImVec2(40, 24))) {
            if (ui_dialogs_handle_save_request(ui, state)) {

                state->show_save_dialog = false;
                state->save_request_name[0] = '\0';
                state->save_error_message[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        }
        theme_pop_button_style();

        if (!can_save) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
        if (ImGui::Button(font_awesome_icon_with_fallback(ICON_FA_XMARK, "Cancel"), ImVec2(40, 24))) {
            state->show_save_dialog = false;
            state->save_request_name[0] = '\0';
            state->save_error_message[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
        theme_pop_button_style();

        if (strlen(state->save_request_name) == 0) {
            ImGui::Spacing();
            theme_render_status_indicator("Please enter a name for the request", STATUS_TYPE_INFO, theme);
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(2); 

    if (state->show_save_dialog && !ImGui::IsPopupOpen("Save Request")) {
        ImGui::OpenPopup("Save Request");
    }
}

void ui_dialogs_render_load_dialog(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(520, 420), ImGuiCond_Appearing);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(SPACING_MD, SPACING_MD));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(SPACING_SM, SPACING_SM));

    if (ImGui::BeginPopupModal("Load Request", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {

        theme_push_header_style();
        font_awesome_render_icon_text(ICON_FA_FOLDER_OPEN, ICON_FALLBACK_FILE, "Load Saved Request", theme->accent_secondary);
        theme_pop_text_style();

        ImGui::Spacing();

        if (state->load_error_message[0] != '\0') {
            theme_render_status_indicator(state->load_error_message, STATUS_TYPE_ERROR, theme);
            ImGui::Spacing();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        ImVec2 window_size = ImGui::GetWindowSize();
        ImVec2 text_size = ImGui::CalcTextSize("Legacy load dialog deprecated");
        ImGui::SetCursorPosX((window_size.x - text_size.x) * 0.5f);

        theme_render_status_indicator("Legacy load dialog deprecated", STATUS_TYPE_INFO, theme);

        ImGui::Spacing();
        ImVec2 help_size = ImGui::CalcTextSize("Use the collections panel instead");
        ImGui::SetCursorPosX((window_size.x - help_size.x) * 0.5f);

        theme_push_caption_style();
        ImGui::TextColored(theme->fg_tertiary, "Use the collections panel instead");
        theme_pop_text_style();

        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::Separator();
        ImGui::Spacing();

        theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
        if (ImGui::Button(font_awesome_icon_with_fallback(ICON_FA_TIMES, "Close"), ImVec2(60, 24))) {
            state->show_load_dialog = false;
            state->load_error_message[0] = '\0';
            state->selected_request_index_for_load = -1;
            ImGui::CloseCurrentPopup();
        }
        theme_pop_button_style();

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(2); 

    if (state->show_load_dialog && !ImGui::IsPopupOpen("Load Request")) {
        ImGui::OpenPopup("Load Request");
    }
}

void ui_dialogs_render_cookie_manager(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(1100, 500), ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(SPACING_LG, SPACING_LG));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(SPACING_MD, SPACING_MD));

    if (ImGui::BeginPopupModal("Manage Cookies", NULL,
                               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {

        theme_push_header_style();
        font_awesome_render_icon_text(ICON_FA_COG, ICON_FALLBACK_SAVE, "Cookie Management", theme->accent_primary);
        theme_pop_text_style();

        ImGui::Spacing();

        Collection* active_collection = app_state_get_active_collection(state);
        if (!active_collection) {

            theme_render_status_indicator("No active collection. Please select or create a collection first.", STATUS_TYPE_WARNING, theme);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
            if (ImGui::Button(font_awesome_icon_with_fallback(ICON_FA_TIMES, "Close"), ImVec2(60, 24))) {
                state->show_cookie_manager = false;
                ImGui::CloseCurrentPopup();
            }
            theme_pop_button_style();

            ImGui::EndPopup();
            ImGui::PopStyleVar(2);
            return;
        }

        CookieJar* cookie_jar = &active_collection->cookie_jar;

        theme_push_body_style();
        ImGui::Text("Collection: %s", active_collection->name);
        ImGui::Text("Cookies: %d", cookie_jar->count);
        theme_pop_text_style();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (cookie_jar->count > 0) {

            theme_push_body_style();
            ImGui::Columns(5, "CookieTable", true);
            ImGui::SetColumnWidth(0, 200.0f); 
            ImGui::SetColumnWidth(1, 400.0f); 
            ImGui::SetColumnWidth(2, 250.0f); 
            ImGui::SetColumnWidth(3, 150.0f); 
            ImGui::SetColumnWidth(4, 100.0f); 

            ImGui::TextColored(theme->accent_secondary, "Name");
            ImGui::NextColumn();
            ImGui::TextColored(theme->accent_secondary, "Value");
            ImGui::NextColumn();
            ImGui::TextColored(theme->accent_secondary, "Domain");
            ImGui::NextColumn();
            ImGui::TextColored(theme->accent_secondary, "Path");
            ImGui::NextColumn();
            ImGui::TextColored(theme->accent_secondary, "Actions");
            ImGui::NextColumn();
            theme_pop_text_style();

            ImGui::Separator();

            static int cookie_to_delete = -1;
            for (int i = 0; i < cookie_jar->count; i++) {
                StoredCookie* cookie = &cookie_jar->cookies[i];

                bool is_expired = cookie_jar_is_cookie_expired(cookie);
                ImVec4 text_color = is_expired ? theme->fg_disabled : theme->fg_primary;

                ImGui::TextColored(text_color, "%s", cookie->name);
                if (is_expired) {
                    ImGui::SameLine();
                    ImGui::TextColored(theme->error, " (expired)");
                }
                ImGui::NextColumn();

                char value_display[128];
                if (strlen(cookie->value) > 80) {
                    strncpy(value_display, cookie->value, 77);
                    value_display[77] = '\0';
                    strcat(value_display, "...");
                } else {
                    strcpy(value_display, cookie->value);
                }
                ImGui::TextColored(text_color, "%s", value_display);
                ImGui::NextColumn();

                const char* domain_display = strlen(cookie->domain) > 0 ? cookie->domain : "(not set)";
                ImGui::TextColored(text_color, "%s", domain_display);
                ImGui::NextColumn();

                const char* path_display = strlen(cookie->path) > 0 ? cookie->path : "/";
                ImGui::TextColored(text_color, "%s", path_display);
                ImGui::NextColumn();

                ImGui::PushID(i);
                theme_push_button_style(theme, BUTTON_TYPE_DANGER);
                if (ImGui::Button(ICON_FA_TRASH, ImVec2(40, 24))) {
                    cookie_to_delete = i;
                }
                theme_pop_button_style();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Delete cookie");
                }
                ImGui::PopID();
                ImGui::NextColumn();
            }

            ImGui::Columns(1); 

            if (cookie_to_delete >= 0) {
                cookie_jar_remove_cookie(cookie_jar, cookie_to_delete);
                cookie_to_delete = -1;

                collection_update_modified_time(active_collection);
                app_state_mark_changed(state);
            }

        } else {
            theme_render_status_indicator("No cookies stored in this collection", STATUS_TYPE_INFO, theme);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        theme_push_button_style(theme, BUTTON_TYPE_DANGER);
        if (ImGui::Button(ICON_FA_TRASH " Clear All", ImVec2(100, 0))) {
            cookie_jar_clear_all(cookie_jar);
            collection_update_modified_time(active_collection);
            app_state_mark_changed(state);
        }
        theme_pop_button_style();

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Delete all cookies from this collection");
        }

        ImGui::SameLine();

        theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
        if (ImGui::Button(ICON_FA_REFRESH " Clean Expired", ImVec2(130, 0))) {
            int removed_count = cookie_jar_cleanup_expired(cookie_jar);
            if (removed_count > 0) {
                collection_update_modified_time(active_collection);
                app_state_mark_changed(state);
                snprintf(state->status_message, sizeof(state->status_message),
                        "Removed %d expired cookies", removed_count);
            } else {
                snprintf(state->status_message, sizeof(state->status_message),
                        "No expired cookies found");
            }
        }
        theme_pop_button_style();

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Remove expired cookies from this collection");
        }

        ImGui::SameLine();

        theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
        if (ImGui::Button(font_awesome_icon_with_fallback(ICON_FA_TIMES, "Close"), ImVec2(40, 0))) {
            state->show_cookie_manager = false;
            ImGui::CloseCurrentPopup();
        }
        theme_pop_button_style();

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(2); 

    if (state->show_cookie_manager && !ImGui::IsPopupOpen("Manage Cookies")) {
        ImGui::OpenPopup("Manage Cookies");
    }
}

bool ui_dialogs_handle_save_request(UIManager* ui, AppState* state) {

    (void)ui; 
    strcpy(state->save_error_message, "Save functionality moved to collections system");
    return false;
}

bool ui_dialogs_handle_load_request(UIManager* ui, AppState* state, int request_index) {

    (void)ui; 
    (void)request_index; 
    strcpy(state->load_error_message, "Load functionality moved to collections system");
    return false;
}

bool ui_dialogs_handle_delete_request(UIManager* ui, AppState* state, int request_index) {

    (void)ui; 
    (void)request_index; 
    strcpy(state->load_error_message, "Delete functionality moved to collections system");
    return false;
}

} 