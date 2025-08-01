/**
 * ui_dialogs.h
 * 
 * modal dialogs and popup interfaces for tinyrequest
 * 
 * this file provides all the modal dialogs used throughout the
 * application - things like save/load dialogs for managing requests,
 * cookie manager for viewing and editing stored cookies, and other
 * popup interfaces that appear on top of the main window.
 * 
 * handles both the rendering of the dialogs and the logic for
 * processing user actions like saving, loading, and deleting requests.
 * 
 * basically all the secondary interfaces that help users manage
 * their data and settings.
 */

#ifndef UI_DIALOGS_H
#define UI_DIALOGS_H

#include <stdbool.h>
#include "ui_manager.h"
#include "app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_dialogs_render_save_dialog(UIManager* ui, AppState* state);
void ui_dialogs_render_load_dialog(UIManager* ui, AppState* state);
void ui_dialogs_render_cookie_manager(UIManager* ui, AppState* state);

bool ui_dialogs_handle_save_request(UIManager* ui, AppState* state);
bool ui_dialogs_handle_load_request(UIManager* ui, AppState* state, int request_index);
bool ui_dialogs_handle_delete_request(UIManager* ui, AppState* state, int request_index);

#ifdef __cplusplus
}
#endif

#endif