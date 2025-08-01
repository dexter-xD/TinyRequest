/**
 * app_window.h
 * 
 * window management and glfw integration for tinyrequest
 * 
 * this file handles creating and managing the main application window
 * using glfw. it sets up the opengl context, handles window events
 * like closing and keyboard input, and provides callbacks for when
 * things go wrong.
 * 
 * pretty much all the low-level window stuff that you need to get
 * a gui application running but don't really want to think about.
 */

#ifndef APP_WINDOW_H
#define APP_WINDOW_H

#include "app/app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int app_window_init(Application* app, int width, int height);
void app_window_cleanup(Application* app);

void app_window_error_callback(int error, const char* description);
void app_window_close_callback(GLFWwindow* window);
void app_window_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

#ifdef __cplusplus
}
#endif

#endif