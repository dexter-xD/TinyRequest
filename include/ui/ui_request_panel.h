/**
 * ui_request_panel.h
 * 
 * request editing interface for tinyrequest
 * 
 * this file handles the main request editing panel where users build
 * their http requests. it includes the url input, method selection,
 * headers editor, request body editor, and the send button.
 * 
 * integrates with the collections system so users can save requests
 * and load them later. also handles keyboard shortcuts for common
 * actions like sending requests and duplicating them.
 * 
 * basically the main workspace where users craft their http requests
 * before sending them out into the world.
 */

#ifndef UI_REQUEST_PANEL_H
#define UI_REQUEST_PANEL_H

#include <stdbool.h>
#include "ui_manager.h"
#include "app_state.h"
#include "request_response.h"
#include "collections.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_request_panel_render(UIManager* ui, AppState* state);
void ui_request_panel_render_request_header(UIManager* ui, AppState* state, Request* request, Collection* collection);
bool ui_request_panel_handle_send_request(UIManager* ui, AppState* state);

bool ui_request_panel_handle_save_request(UIManager* ui, AppState* state);
bool ui_request_panel_handle_duplicate_request(UIManager* ui, AppState* state);

void ui_request_panel_render_body_panel(UIManager* ui, AppState* state);
void ui_request_panel_render_headers_panel(UIManager* ui, AppState* state, HeaderList* headers);

void ui_request_panel_handle_keyboard_shortcuts(UIManager* ui, AppState* state);

#ifdef __cplusplus
}
#endif

#endif