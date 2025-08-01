/**
 * ui_panels.h
 * 
 * reusable ui panel components for tinyrequest
 * 
 * this file provides common ui panels that are used throughout the
 * application - things like the headers panel for managing http headers
 * and the body panel for editing request/response content.
 * 
 * these are the building blocks that get composed together to create
 * the larger interfaces like the request and response tabs.
 */

#ifndef UI_PANELS_H
#define UI_PANELS_H

#include <stddef.h>
#include "ui_manager.h"
#include "request_response.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_panels_render_headers_panel(UIManager* ui, HeaderList* headers, void* app_state);
void ui_panels_render_body_panel(UIManager* ui, char* body_buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif