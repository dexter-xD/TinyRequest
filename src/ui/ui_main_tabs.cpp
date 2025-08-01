#include "ui/ui_main_tabs.h"
#include "ui/ui_manager.h"
#include "ui/theme.h"
#include "app_state.h"
#include "ui/ui_request_panel.h"
#include "ui/ui_response_panel.h"
#include "font_awesome.h"
#include "persistence.h"
#include <string.h>
#include <stdio.h>

extern "C" {

void ui_main_tabs_render(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    ImVec2 content_size = ImGui::GetContentRegionAvail();

    float collections_width = content_size.x * 0.25f;
    float request_width = content_size.x * 0.40f;
    float response_width = content_size.x * 0.35f;

    collections_width = (collections_width < 250.0f) ? 250.0f : collections_width;
    request_width = (request_width < 400.0f) ? 400.0f : request_width;
    response_width = (response_width < 300.0f) ? 300.0f : response_width;

    ImGui::BeginChild("CollectionsPanel", ImVec2(collections_width, 0), true, ImGuiWindowFlags_None);
    ui_main_tabs_render_collections_tab(ui, state);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("RequestPanel", ImVec2(request_width, 0), true, ImGuiWindowFlags_None);
    ui_main_tabs_render_request_tab(ui, state);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("ResponsePanel", ImVec2(response_width, 0), true, ImGuiWindowFlags_None);
    ui_main_tabs_render_response_tab(ui, state);
    ImGui::EndChild();
}

void ui_main_tabs_render_collections_tab(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

    ImGui::PushStyleColor(ImGuiCol_Text, theme->accent_primary);
    if (state->collection_manager && state->collection_manager->count > 0) {
        ImGui::Text(ICON_FA_FOLDER " Collections (%d)", state->collection_manager->count);
    } else {
        ImGui::Text(ICON_FA_FOLDER " Collections");
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    float button_width = 40.0f;
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 48.0f - 8.0f);
    theme_push_button_style(theme, BUTTON_TYPE_PRIMARY);
    if (ImGui::Button(ICON_FA_PLUS, ImVec2(40, 24))) {
        state->show_collection_create_dialog = true;
        app_state_clear_ui_buffers(state);
    }
    theme_pop_button_style();
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Create New Collection");
    }

    ImGui::PopStyleVar();
    ImGui::Separator();
    ImGui::Spacing();

    if (!state->collection_manager || state->collection_manager->count == 0) {
        ui_collections_render_empty_state(ui, state, theme);
    } else {
        ui_collections_render_tree_view(ui, state, theme);
    }

    ui_collections_render_create_dialog(ui, state);
    ui_collections_render_rename_dialog(ui, state);
    ui_collections_render_request_create_dialog(ui, state);
}

void ui_main_tabs_render_request_tab(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    Request* active_request = app_state_get_active_request(state);

    ImGui::PushStyleColor(ImGuiCol_Text, theme->accent_primary);
    ImGui::Text(ICON_FA_COG " Request Configuration");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    if (!active_request) {

        ImVec2 window_size = ImGui::GetContentRegionAvail();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + window_size.y * 0.15f);

        ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_tertiary);
        const char* icon_text = ICON_FA_ARROW_RIGHT;
        ImVec2 icon_size = ImGui::CalcTextSize(icon_text);
        ImGui::SetCursorPosX((window_size.x - icon_size.x) * 0.5f);
        ImGui::Text("%s", icon_text);
        ImGui::PopStyleColor();

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_secondary);
        const char* main_text = "No request selected";
        ImVec2 main_size = ImGui::CalcTextSize(main_text);
        ImGui::SetCursorPosX((window_size.x - main_size.x) * 0.5f);
        ImGui::Text("%s", main_text);
        ImGui::PopStyleColor();

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_disabled);
        const char* help_text = "Select a request from Collections or create a \n\t\t\t\t\t\t new collection to get started";
        ImVec2 help_size = ImGui::CalcTextSize(help_text);
        ImGui::SetCursorPosX((window_size.x - help_size.x) * 0.5f);
        ImGui::Text("%s", help_text);
        ImGui::PopStyleColor();

    } else {

        static int last_active_request = -1;
        static int last_active_collection = -1;

        CollectionManager* manager = state->collection_manager;
        if (manager && (last_active_request != manager->active_request_index ||
                       last_active_collection != manager->active_collection_index)) {

            app_state_mark_request_dirty(state);
            last_active_request = manager->active_request_index;
            last_active_collection = manager->active_collection_index;
        }

        ui_request_panel_render(ui, state);
    }
}

void ui_main_tabs_render_response_tab(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    ImGui::PushStyleColor(ImGuiCol_Text, theme->accent_primary);
    ImGui::Text(ICON_FA_DOWNLOAD " Response Details");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    if (state->current_response.status_code == 0 && !state->request_in_progress) {

        ImVec2 window_size = ImGui::GetContentRegionAvail();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + window_size.y * 0.15f);

        ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_tertiary);
        const char* icon_text = ICON_FA_DOWNLOAD;
        ImVec2 icon_size = ImGui::CalcTextSize(icon_text);
        ImGui::SetCursorPosX((window_size.x - icon_size.x) * 0.5f);
        ImGui::Text("%s", icon_text);
        ImGui::PopStyleColor();

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_secondary);
        const char* main_text = "No response received yet";
        ImVec2 main_size = ImGui::CalcTextSize(main_text);
        ImGui::SetCursorPosX((window_size.x - main_size.x) * 0.5f);
        ImGui::Text("%s", main_text);
        ImGui::PopStyleColor();

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_disabled);
        const char* help_text = "Configure your request and click 'Send Request' \n\t\t\t\t\t\t\t\t\t\t\t\t\t to see the response!";
        ImVec2 help_size = ImGui::CalcTextSize(help_text);
        ImGui::SetCursorPosX((window_size.x - help_size.x) * 0.5f);
        ImGui::Text("%s", help_text);
        ImGui::PopStyleColor();

    } else {

        ui_response_panel_render(ui, state);
    }
}

void ui_main_tabs_handle_tab_switch(UIManager* ui, AppState* state, MainTab new_tab) {
    if (!ui || !state) {
        return;
    }

    if (!ui_main_tabs_should_show_tab(state, new_tab)) {
        return;
    }

    app_state_set_active_tab(state, new_tab);

    switch (new_tab) {
        case TAB_COLLECTIONS:

            break;

        case TAB_REQUEST:

            ui_manager_update_from_state(ui, state);
            break;

        case TAB_RESPONSE:

            break;
    }
}

bool ui_main_tabs_should_show_tab(AppState* state, MainTab tab) {
    if (!state) {
        return false;
    }

    switch (tab) {
        case TAB_COLLECTIONS:
            return true;

        case TAB_REQUEST:
            return true;

        case TAB_RESPONSE:
            return true;

        default:
            return false;
    }
}

void ui_main_tabs_push_tab_style(bool is_active) {
    const ModernGruvboxTheme* theme = theme_get_current();

    if (is_active) {
        ImGui::PushStyleColor(ImGuiCol_Tab, theme->accent_primary);
        ImGui::PushStyleColor(ImGuiCol_Text, theme->bg_normal);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Tab, theme->bg_panel);
        ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_secondary);
    }
}

void ui_main_tabs_pop_tab_style(void) {
    ImGui::PopStyleColor(2);
}

void ui_collections_render_empty_state(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme) {

    ImVec2 window_size = ImGui::GetContentRegionAvail();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + window_size.y * 0.2f);

    ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_tertiary);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); 
    const char* icon_text = ICON_FA_FOLDER_OPEN;
    ImVec2 icon_size = ImGui::CalcTextSize(icon_text);
    ImGui::SetCursorPosX((window_size.x - icon_size.x) * 0.5f);
    ImGui::Text("%s", icon_text);
    ImGui::PopFont();
    ImGui::PopStyleColor();

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_secondary);
    const char* main_text = "No collections yet";
    ImVec2 main_size = ImGui::CalcTextSize(main_text);
    ImGui::SetCursorPosX((window_size.x - main_size.x) * 0.5f);
    ImGui::Text("%s", main_text);
    ImGui::PopStyleColor();

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_disabled);
    const char* help_text = "Create your first collection to\norganize your HTTP requests";
    ImVec2 help_size = ImGui::CalcTextSize(help_text);
    ImGui::SetCursorPosX((window_size.x - help_size.x) * 0.5f);
    ImGui::Text("%s", help_text);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Spacing();

    theme_push_button_style(theme, BUTTON_TYPE_PRIMARY);
    ImVec2 button_size = ImVec2(160, 32);
    ImGui::SetCursorPosX((window_size.x - button_size.x) * 0.5f);
    if (ImGui::Button(ICON_FA_PLUS " New Collection", button_size)) {
        state->show_collection_create_dialog = true;
        app_state_clear_ui_buffers(state);
    }
    theme_pop_button_style();
}

void ui_collections_render_tree_view(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme) {

    ImGui::BeginChild("CollectionsTreeView", ImVec2(0, 0), false, ImGuiWindowFlags_None);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);

    CollectionManager* manager = state->collection_manager;

    for (int i = 0; i < manager->count; i++) {
        ui_collections_render_collection_node(ui, state, theme, i);
    }

    ImGui::EndChild();
}

void ui_collections_render_collection_node(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme, int collection_index) {
    CollectionManager* manager = state->collection_manager;
    Collection* collection = collection_manager_get_collection(manager, collection_index);

    if (!collection) {
        return;
    }

    ImGui::PushID(collection_index);

    bool is_active_collection = (manager->active_collection_index == collection_index);
    bool is_expanded = false;

    if (is_active_collection) {
        ImGui::PushStyleColor(ImGuiCol_Header, theme_alpha_blend(theme->accent_primary, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, theme_alpha_blend(theme->accent_primary, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, theme_alpha_blend(theme->accent_primary, 0.5f));
    }

    char collection_label[512];
    snprintf(collection_label, sizeof(collection_label), "%s %s (%d)", 
             ICON_FA_FOLDER, collection->name, collection->request_count);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (collection->request_count == 0) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    is_expanded = ImGui::TreeNodeEx(collection_label, flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        printf("DEBUG: ui_collections_render_collection_node - collection clicked: index=%d, name='%s'\n",
               collection_index, collection->name);
        app_state_set_active_collection(state, collection_index);

        app_state_set_active_tab(state, TAB_REQUEST);
        printf("DEBUG: ui_collections_render_collection_node - collection selection completed\n");
    }

    if (ImGui::BeginPopupContextItem()) {
        ui_collections_render_collection_context_menu(ui, state, collection_index);
        ImGui::EndPopup();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("REQUEST_MOVE")) {
            struct {
                int source_collection;
                int request_index;
            } move_data = *(const decltype(move_data)*)payload->Data;

            if (move_data.source_collection != collection_index) {
                ui_collections_handle_move_request(ui, state, 
                                                  move_data.source_collection, 
                                                  move_data.request_index, 
                                                  collection_index);
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (is_active_collection) {
        ImGui::PopStyleColor(3);
    }

    if (is_expanded) {
        for (int j = 0; j < collection->request_count; j++) {
            ui_collections_render_request_node(ui, state, theme, collection_index, j);
        }

        if (collection->request_count == 0) {
            ImGui::Indent();
            ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_disabled);
            ImGui::Text(ICON_FA_FILE " No requests in this collection");
            ImGui::PopStyleColor();
            ImGui::Unindent();
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void ui_collections_render_request_node(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme, int collection_index, int request_index) {
    CollectionManager* manager = state->collection_manager;
    Collection* collection = collection_manager_get_collection(manager, collection_index);

    if (!collection || request_index >= collection->request_count) {
        return;
    }

    ImGui::PushID(request_index);

    const char* request_name = collection_get_request_name(collection, request_index);
    Request* request = collection_get_request(collection, request_index);

    if (!request_name || !request) {
        ImGui::PopID();
        return;
    }

    bool is_active_request = (manager->active_collection_index == collection_index && 
                             manager->active_request_index == request_index);

    if (is_active_request) {
        ImGui::PushStyleColor(ImGuiCol_Header, theme_alpha_blend(theme->accent_secondary, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, theme_alpha_blend(theme->accent_secondary, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, theme_alpha_blend(theme->accent_secondary, 0.5f));
    }

    char request_label[512];
    snprintf(request_label, sizeof(request_label), "%s [%s] %s", 
             ICON_FA_FILE_TEXT, request->method, request_name);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    ImGui::TreeNodeEx(request_label, flags);

    if (ImGui::IsItemClicked()) {
        app_state_set_active_collection(state, collection_index);
        app_state_set_active_request(state, request_index);

        app_state_set_active_tab(state, TAB_REQUEST);
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {

        struct {
            int source_collection;
            int request_index;
        } payload = { collection_index, request_index };

        ImGui::SetDragDropPayload("REQUEST_MOVE", &payload, sizeof(payload));

        ImGui::Text("Move: %s", request_name);
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginPopupContextItem()) {
        ui_collections_render_request_context_menu(ui, state, collection_index, request_index);
        ImGui::EndPopup();
    }

    if (is_active_request) {
        ImGui::PopStyleColor(3);
    }

    ImGui::PopID();
}

void ui_collections_render_create_dialog(UIManager* ui, AppState* state) {
    if (!state->show_collection_create_dialog) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 320), ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

    if (ImGui::BeginPopupModal("Create Collection", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {

        ImGui::PushStyleColor(ImGuiCol_Text, theme->accent_primary);
        ImGui::Text(ICON_FA_FOLDER_PLUS " Create New Collection");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Collection Name:");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##collection_name", state->collection_name_buffer, sizeof(state->collection_name_buffer));

        ImGui::Spacing();

        ImGui::Text("Description (optional):");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextMultiline("##collection_description", state->collection_description_buffer, 
                                 sizeof(state->collection_description_buffer), ImVec2(0, 60));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool can_create = strlen(state->collection_name_buffer) > 0;

        if (!can_create) {
            ImGui::BeginDisabled();
        }

        theme_push_button_style(theme, BUTTON_TYPE_SUCCESS);
        if (ImGui::Button(ICON_FA_CHECK " Create", ImVec2(80, 0))) {
            if (ui_collections_handle_create_collection(ui, state)) {
                state->show_collection_create_dialog = false;
                app_state_clear_ui_buffers(state);
                ImGui::CloseCurrentPopup();
            }
        }
        theme_pop_button_style();

        if (!can_create) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
        if (ImGui::Button(ICON_FA_TIMES " Cancel", ImVec2(80, 0))) {
            state->show_collection_create_dialog = false;
            app_state_clear_ui_buffers(state);
            ImGui::CloseCurrentPopup();
        }
        theme_pop_button_style();

        if (strlen(state->collection_name_buffer) == 0) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, theme->fg_disabled);
            ImGui::Text("Please enter a collection name");
            ImGui::PopStyleColor();
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();

    if (!ImGui::IsPopupOpen("Create Collection")) {
        ImGui::OpenPopup("Create Collection");
    }
}

void ui_collections_render_rename_dialog(UIManager* ui, AppState* state) {
    if (!state->show_collection_rename_dialog) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 180), ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

    if (ImGui::BeginPopupModal("Rename Collection", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {

        ImGui::PushStyleColor(ImGuiCol_Text, theme->accent_primary);
        ImGui::Text(ICON_FA_EDIT " Rename Collection");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("New Name:");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##rename_collection_name", state->collection_name_buffer, sizeof(state->collection_name_buffer));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool can_rename = strlen(state->collection_name_buffer) > 0;

        if (!can_rename) {
            ImGui::BeginDisabled();
        }

        theme_push_button_style(theme, BUTTON_TYPE_SUCCESS);
        if (ImGui::Button(ICON_FA_CHECK " Rename", ImVec2(80, 0))) {
            CollectionManager* manager = state->collection_manager;
            if (ui_collections_handle_rename_collection(ui, state, manager->active_collection_index)) {
                state->show_collection_rename_dialog = false;
                app_state_clear_ui_buffers(state);
                ImGui::CloseCurrentPopup();
            }
        }
        theme_pop_button_style();

        if (!can_rename) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
        if (ImGui::Button(ICON_FA_TIMES " Cancel", ImVec2(80, 0))) {
            state->show_collection_rename_dialog = false;
            app_state_clear_ui_buffers(state);
            ImGui::CloseCurrentPopup();
        }
        theme_pop_button_style();

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();

    if (!ImGui::IsPopupOpen("Rename Collection")) {
        ImGui::OpenPopup("Rename Collection");
    }
}

void ui_collections_render_request_create_dialog(UIManager* ui, AppState* state) {
    if (!state->show_request_create_dialog) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

    if (ImGui::BeginPopupModal("Create Request", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {

        ImGui::PushStyleColor(ImGuiCol_Text, theme->accent_primary);
        ImGui::Text(ICON_FA_PLUS " Create New Request");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Request Name:");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##request_name", state->request_name_buffer, sizeof(state->request_name_buffer));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool can_create = strlen(state->request_name_buffer) > 0;

        if (!can_create) {
            ImGui::BeginDisabled();
        }

        theme_push_button_style(theme, BUTTON_TYPE_SUCCESS);
        if (ImGui::Button(ICON_FA_CHECK " Create", ImVec2(80, 0))) {
            CollectionManager* manager = state->collection_manager;
            if (ui_collections_handle_create_request(ui, state, manager->active_collection_index)) {
                state->show_request_create_dialog = false;
                ImGui::CloseCurrentPopup();
            }
        }
        theme_pop_button_style();

        if (!can_create) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        theme_push_button_style(theme, BUTTON_TYPE_NORMAL);
        if (ImGui::Button(ICON_FA_TIMES " Cancel", ImVec2(80, 0))) {
            state->show_request_create_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        theme_pop_button_style();

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();

    if (!ImGui::IsPopupOpen("Create Request")) {
        ImGui::OpenPopup("Create Request");
    }
}

void ui_collections_render_collection_context_menu(UIManager* ui, AppState* state, int collection_index) {
    CollectionManager* manager = state->collection_manager;
    Collection* collection = collection_manager_get_collection(manager, collection_index);

    if (!collection) {
        return;
    }

    const ModernGruvboxTheme* theme = theme_get_current();

    if (ImGui::MenuItem(ICON_FA_PLUS " Add Request")) {
        app_state_set_active_collection(state, collection_index);
        state->show_request_create_dialog = true;
    }

    ImGui::Separator();

    if (ImGui::MenuItem(ICON_FA_EDIT " Rename")) {
        app_state_set_active_collection(state, collection_index);
        state->show_collection_rename_dialog = true;

        strncpy(state->collection_name_buffer, collection->name, sizeof(state->collection_name_buffer) - 1);
        state->collection_name_buffer[sizeof(state->collection_name_buffer) - 1] = '\0';
    }

    if (ImGui::MenuItem(ICON_FA_COPY " Duplicate")) {
        collection_manager_duplicate_collection(manager, collection_index);
        app_state_save_all_collections(state);
    }

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, theme->status_error);
    if (ImGui::MenuItem(ICON_FA_TRASH " Delete")) {
        ui_collections_handle_delete_collection(ui, state, collection_index);
    }
    ImGui::PopStyleColor();
}

void ui_collections_render_request_context_menu(UIManager* ui, AppState* state, int collection_index, int request_index) {
    const ModernGruvboxTheme* theme = theme_get_current();

    if (ImGui::MenuItem(ICON_FA_COPY " Duplicate")) {
        ui_collections_handle_duplicate_request(ui, state, collection_index, request_index);
    }

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, theme->status_error);
    if (ImGui::MenuItem(ICON_FA_TRASH " Delete")) {
        ui_collections_handle_delete_request(ui, state, collection_index, request_index);
    }
    ImGui::PopStyleColor();
}

bool ui_collections_handle_create_collection(UIManager* ui, AppState* state) {
    if (!state || !state->collection_manager) {
        return false;
    }

    if (!collection_validate_name(state->collection_name_buffer)) {
        return false;
    }

    Collection* new_collection = collection_create(state->collection_name_buffer, state->collection_description_buffer);
    if (!new_collection) {
        return false;
    }

    int index = collection_manager_add_collection(state->collection_manager, new_collection);
    collection_destroy(new_collection); 

    if (index < 0) {
        return false;
    }

    app_state_set_active_collection(state, index);

    app_state_save_all_collections(state);

    snprintf(state->status_message, sizeof(state->status_message), 
             "Collection '%s' created successfully", state->collection_name_buffer);

    return true;
}

bool ui_collections_handle_rename_collection(UIManager* ui, AppState* state, int collection_index) {
    if (!state || !state->collection_manager) {
        return false;
    }

    Collection* collection = collection_manager_get_collection(state->collection_manager, collection_index);
    if (!collection) {
        return false;
    }

    if (!collection_validate_name(state->collection_name_buffer)) {
        return false;
    }

    if (collection_set_name(collection, state->collection_name_buffer) != 0) {
        return false;
    }

    app_state_save_all_collections(state);

    snprintf(state->status_message, sizeof(state->status_message), 
             "Collection renamed to '%s'", state->collection_name_buffer);

    return true;
}

bool ui_collections_handle_delete_collection(UIManager* ui, AppState* state, int collection_index) {
    if (!state || !state->collection_manager) {
        return false;
    }

    Collection* collection = collection_manager_get_collection(state->collection_manager, collection_index);
    if (!collection) {
        return false;
    }

    char collection_name[256];
    char collection_id[64];
    strncpy(collection_name, collection->name, sizeof(collection_name) - 1);
    collection_name[sizeof(collection_name) - 1] = '\0';
    strncpy(collection_id, collection->id, sizeof(collection_id) - 1);
    collection_id[sizeof(collection_id) - 1] = '\0';

    if (collection_manager_remove_collection(state->collection_manager, collection_index) != 0) {
        return false;
    }

    persistence_delete_collection_file(collection_id);

    app_state_save_all_collections(state);

    snprintf(state->status_message, sizeof(state->status_message),
             "Collection '%s' deleted", collection_name);

    return true;
}

bool ui_collections_handle_create_request(UIManager* ui, AppState* state, int collection_index) {
    if (!state || !state->collection_manager) {
        return false;
    }

    Collection* collection = collection_manager_get_collection(state->collection_manager, collection_index);
    if (!collection) {
        return false;
    }

    Request new_request;
    request_init(&new_request);
    strcpy(new_request.method, "GET");
    strcpy(new_request.url, "https://");

    int request_index = collection_add_request(collection, &new_request, state->request_name_buffer);
    request_cleanup(&new_request);

    if (request_index < 0) {
        return false;
    }

    app_state_set_active_collection(state, collection_index);
    app_state_set_active_request(state, request_index);

    app_state_mark_request_dirty(state);

    app_state_set_active_tab(state, TAB_REQUEST);

    app_state_save_all_collections(state);

    snprintf(state->status_message, sizeof(state->status_message), 
             "Request '%s' created", state->request_name_buffer);

    return true;
}

bool ui_collections_handle_delete_request(UIManager* ui, AppState* state, int collection_index, int request_index) {
    if (!state || !state->collection_manager) {
        return false;
    }

    Collection* collection = collection_manager_get_collection(state->collection_manager, collection_index);
    if (!collection) {
        return false;
    }

    const char* request_name = collection_get_request_name(collection, request_index);
    char name_copy[256];
    if (request_name) {
        strncpy(name_copy, request_name, sizeof(name_copy) - 1);
        name_copy[sizeof(name_copy) - 1] = '\0';
    } else {
        strcpy(name_copy, "Unnamed Request");
    }

    if (collection_remove_request(collection, request_index) != 0) {
        return false;
    }

    CollectionManager* manager = state->collection_manager;
    if (manager->active_collection_index == collection_index && 
        manager->active_request_index == request_index) {

        int new_request_index = (request_index > 0) ? request_index - 1 : 
                               (collection->request_count > 0) ? 0 : -1;
        app_state_set_active_request(state, new_request_index);
    } else if (manager->active_collection_index == collection_index && 
               manager->active_request_index > request_index) {

        app_state_set_active_request(state, manager->active_request_index - 1);
    }

    app_state_save_all_collections(state);

    snprintf(state->status_message, sizeof(state->status_message), 
             "Request '%s' deleted", name_copy);

    return true;
}

bool ui_collections_handle_duplicate_request(UIManager* ui, AppState* state, int collection_index, int request_index) {
    if (!state || !state->collection_manager) {
        return false;
    }

    Collection* collection = collection_manager_get_collection(state->collection_manager, collection_index);
    if (!collection) {
        return false;
    }

    int new_index = collection_duplicate_request(collection, request_index);
    if (new_index < 0) {
        return false;
    }

    app_state_set_active_collection(state, collection_index);
    app_state_set_active_request(state, new_index);

    app_state_save_all_collections(state);

    const char* request_name = collection_get_request_name(collection, request_index);
    snprintf(state->status_message, sizeof(state->status_message), 
             "Request '%s' duplicated", request_name ? request_name : "Unnamed Request");

    return true;
}

bool ui_collections_handle_move_request(UIManager* ui, AppState* state, int source_collection, int request_index, int target_collection) {
    if (!state || !state->collection_manager) {
        return false;
    }

    CollectionManager* manager = state->collection_manager;
    Collection* source_col = collection_manager_get_collection(manager, source_collection);
    Collection* target_col = collection_manager_get_collection(manager, target_collection);

    if (!source_col || !target_col || request_index < 0 || request_index >= source_col->request_count) {
        return false;
    }

    Request* request_to_move = collection_get_request(source_col, request_index);
    const char* request_name = collection_get_request_name(source_col, request_index);

    if (!request_to_move || !request_name) {
        return false;
    }

    int new_index = collection_add_request(target_col, request_to_move, request_name);
    if (new_index < 0) {
        return false;
    }

    if (collection_remove_request(source_col, request_index) != 0) {

        collection_remove_request(target_col, new_index);
        return false;
    }

    if (manager->active_collection_index == source_collection && 
        manager->active_request_index == request_index) {
        app_state_set_active_collection(state, target_collection);
        app_state_set_active_request(state, new_index);
    } else if (manager->active_collection_index == source_collection && 
               manager->active_request_index > request_index) {

        app_state_set_active_request(state, manager->active_request_index - 1);
    }

    app_state_save_all_collections(state);

    snprintf(state->status_message, sizeof(state->status_message), 
             "Request '%s' moved to '%s'", request_name, target_col->name);

    return true;
}

} 