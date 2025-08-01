/*
 * app_types.h
 * 
 * core application structures for tinyrequest - a lightweight http client
 * 
 * this file defines the main application container that holds everything
 * together. the application struct manages the window, keeps track of
 * whether we're still running, and connects the ui with the backend state.
 * 
 * pretty straightforward stuff - just the basic building blocks that
 * everything else depends on.
 */

#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <GLFW/glfw3.h>
#include <stdbool.h>
#include "ui/ui_manager.h"
#include "app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/* main application container - holds everything together */
typedef struct {
    GLFWwindow* window;      /* the main window handle */
    bool running;            /* whether the app should keep going */
    AppState* state;         /* all the application data and logic */
    UIManager* ui_manager;   /* handles rendering and user interaction */
} Application;

#ifdef __cplusplus
}
#endif

#endif