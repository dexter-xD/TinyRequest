/**
 * ui_core.h
 * 
 * core user interface management for tinyrequest
 * 
 * this file handles the main ui lifecycle - creating and destroying
 * the imgui context, initializing the rendering system, and coordinating
 * the overall ui rendering process.
 * 
 * also provides utility functions for converting between different
 * representations of http methods and updating the ui based on
 * application state changes.
 * 
 * basically the foundation that everything else in the ui builds on top of.
 */

#ifndef UI_CORE_H
#define UI_CORE_H

#include <GLFW/glfw3.h>
#include "ui_manager.h"
#include "app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

UIManager* ui_core_create(void);
void ui_core_destroy(UIManager* ui);
int ui_core_init(UIManager* ui, GLFWwindow* window);
void ui_core_cleanup(UIManager* ui);
void ui_core_render(UIManager* ui, AppState* state);

void ui_core_update_from_state(UIManager* ui, const AppState* state);
const char* ui_core_get_method_string(int method_index);
int ui_core_get_method_index(const char* method);

#ifdef __cplusplus
}
#endif

#endif