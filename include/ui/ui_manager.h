/**
 * ui_manager.h
 * 
 * main ui coordination and rendering for tinyrequest
 * 
 * this file manages the overall user interface, coordinating between
 * different panels and handling the main rendering loop. it contains
 * the ui manager structure that holds the imgui context and provides
 * functions for rendering all the different parts of the interface.
 * 
 * handles everything from the main request/response panels to dialogs
 * for saving and loading requests. also manages ui events like button
 * clicks and form submissions.
 * 
 * basically the conductor that makes sure all the different ui pieces
 * work together smoothly.
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <GLFW/glfw3.h>
#include "imgui.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ImGuiContext* imgui_context;
    ImGuiIO* io;
} UIManager;

UIManager* ui_manager_create(void);
void ui_manager_destroy(UIManager* ui);
int ui_manager_init(UIManager* ui, GLFWwindow* window);
void ui_manager_cleanup(UIManager* ui);

void ui_manager_render(UIManager* ui, void* state);
void ui_manager_render_request_panel(UIManager* ui, void* state);
void ui_manager_render_response_panel(UIManager* ui, void* state);
void ui_manager_render_headers_panel(UIManager* ui, void* headers, void* app_state);
void ui_manager_render_body_panel(UIManager* ui, char* body_buffer, size_t buffer_size);
void ui_manager_render_save_dialog(UIManager* ui, void* state);
void ui_manager_render_load_dialog(UIManager* ui, void* state);

bool ui_manager_handle_send_request(UIManager* ui, void* state);
void ui_manager_update_from_state(UIManager* ui, const void* state);
bool ui_manager_handle_save_request(UIManager* ui, void* state);
bool ui_manager_handle_load_request(UIManager* ui, void* state, int request_index);
bool ui_manager_handle_delete_request(UIManager* ui, void* state, int request_index);

const char* ui_manager_get_method_string(int method_index);
int ui_manager_get_method_index(const char* method);

void ui_manager_sync_with_active_request(UIManager* ui, void* state);

#ifdef __cplusplus
}
#endif

#endif