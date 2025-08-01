/**
 * ui_response_panel.h
 * 
 * response viewing interface for tinyrequest
 * 
 * this file handles displaying http responses after requests are sent.
 * it shows the status code, response headers, response body, and timing
 * information in a clean, readable format.
 * 
 * includes syntax highlighting for json and xml responses, proper
 * handling of large responses that might need scrolling, and cleanup
 * functions for managing memory used by response data.
 * 
 * basically where users go to see what came back from their http requests.
 */

#ifndef UI_RESPONSE_PANEL_H
#define UI_RESPONSE_PANEL_H

#include "ui_manager.h"
#include "app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_response_panel_render(UIManager* ui, AppState* state);

void ui_response_panel_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif