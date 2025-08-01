/**
 * app_core.h
 * 
 * core application lifecycle management for tinyrequest
 * 
 * this file contains the main functions that control the application's
 * lifecycle - starting up, running the main loop, and shutting down
 * cleanly. it's the entry point that ties together all the different
 * parts of the application like the ui, state management, and window
 * handling.
 * 
 * basically the conductor that makes sure everything starts up in the
 * right order and keeps running smoothly until the user decides to quit.
 */

#ifndef APP_CORE_H
#define APP_CORE_H

#include "app/app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int app_core_init(Application* app);
void app_core_cleanup(Application* app);
void app_core_run_main_loop(Application* app);
void app_core_update_window_title(Application* app);

#ifdef __cplusplus
}
#endif

#endif