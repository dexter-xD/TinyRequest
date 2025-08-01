#include "ui/ui_manager.h"
#include "ui/ui_core.h"
#include "ui/ui_request_panel.h"
#include "ui/ui_response_panel.h"
#include "ui/ui_dialogs.h"
#include "ui/ui_panels.h"
#include "app_state.h"
#include "request_response.h"

extern "C" {

/* Wrapper functions that delegate to the modular UI components */ 

UIManager* ui_manager_create(void) {
    return ui_core_create();
}

void ui_manager_destroy(UIManager* ui) {
    ui_core_destroy(ui);
}

int ui_manager_init(UIManager* ui, GLFWwindow* window) {
    return ui_core_init(ui, window);
}

void ui_manager_cleanup(UIManager* ui) {
    ui_core_cleanup(ui);
}

void ui_manager_render(UIManager* ui, void* state) {
    ui_core_render(ui, (AppState*)state);
}

void ui_manager_render_request_panel(UIManager* ui, void* state) {
    ui_request_panel_render(ui, (AppState*)state);
}

void ui_manager_render_response_panel(UIManager* ui, void* state) {
    ui_response_panel_render(ui, (AppState*)state);
}

void ui_manager_render_headers_panel(UIManager* ui, void* headers, void* app_state) {
    ui_panels_render_headers_panel(ui, (HeaderList*)headers, app_state);
}

void ui_manager_render_body_panel(UIManager* ui, char* body_buffer, size_t buffer_size) {
    ui_panels_render_body_panel(ui, body_buffer, buffer_size);
}

bool ui_manager_handle_send_request(UIManager* ui, void* state) {
    return ui_request_panel_handle_send_request(ui, (AppState*)state);
}

void ui_manager_update_from_state(UIManager* ui, const void* state) {
    ui_core_update_from_state(ui, (const AppState*)state);
}

const char* ui_manager_get_method_string(int method_index) {
    return ui_core_get_method_string(method_index);
}

int ui_manager_get_method_index(const char* method) {
    return ui_core_get_method_index(method);
}

void ui_manager_render_save_dialog(UIManager* ui, void* state) {
    ui_dialogs_render_save_dialog(ui, (AppState*)state);
}

bool ui_manager_handle_save_request(UIManager* ui, void* state) {
    return ui_dialogs_handle_save_request(ui, (AppState*)state);
}

void ui_manager_render_load_dialog(UIManager* ui, void* state) {
    ui_dialogs_render_load_dialog(ui, (AppState*)state);
}

bool ui_manager_handle_load_request(UIManager* ui, void* state, int request_index) {
    return ui_dialogs_handle_load_request(ui, (AppState*)state, request_index);
}

bool ui_manager_handle_delete_request(UIManager* ui, void* state, int request_index) {
    return ui_dialogs_handle_delete_request(ui, (AppState*)state, request_index);
}

void ui_manager_sync_with_active_request(UIManager* ui, void* state) {
    if (!ui || !state) {
        return;
    }
    
    AppState* app_state = (AppState*)state;
    /*State synchronization is now handled by AppState functions*/ 
    app_state_sync_request_to_ui(app_state);
}

} 