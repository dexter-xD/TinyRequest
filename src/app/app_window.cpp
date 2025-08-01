/*
 * handles the main application window creation and management using glfw
 * sets up window callbacks for keyboard shortcuts and window events
 * manages window lifecycle from initialization to cleanup
 */

#include "app/app_window.h"
#include "app/app_types.h"
#include "ui/ui_manager.h"
#include "ui/ui_request_panel.h"
#include "app_state.h"
#include <stdio.h>
#include <GLFW/glfw3.h>

/* handles glfw error messages by printing them to console */
void app_window_error_callback(int error, const char* description) {
    printf("Error %d: %s\n", error, description);
}

/* handles window close events by setting the application running flag to false */
void app_window_close_callback(GLFWwindow* window) {
    Application* app = (Application*)glfwGetWindowUserPointer(window);
    if (app) {
        app->running = false;
    }
}

/* processes keyboard input and handles application shortcuts like ctrl+q, ctrl+r, ctrl+s */
void app_window_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    Application* app = (Application*)glfwGetWindowUserPointer(window);
    if (!app || !app->state) {
        return;
    }
    
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_Q && (mods & GLFW_MOD_CONTROL)) {
            app->running = false;
            return;
        }
        
        if (key == GLFW_KEY_R && (mods & GLFW_MOD_CONTROL)) {
            if (!app->state->request_in_progress) {
                ui_manager_handle_send_request(app->ui_manager, app->state);
            }
            return;
        }
        
        if (key == GLFW_KEY_S && (mods & GLFW_MOD_CONTROL)) {
            Request* active_request = app_state_get_active_request(app->state);
            if (active_request) {
                ui_request_panel_handle_save_request(app->ui_manager, app->state);
            } else {
                app->state->show_save_dialog = true;
                app->state->save_error_message[0] = '\0';
            }
            return;
        }
        
        if (key == GLFW_KEY_O && (mods & GLFW_MOD_CONTROL)) {
            app->state->show_load_dialog = true;
            app->state->load_error_message[0] = '\0';
            app->state->selected_request_index_for_load = -1;
            return;
        }
        
        if (key == GLFW_KEY_ESCAPE) {
            app->state->show_save_dialog = false;
            app->state->show_load_dialog = false;
            return;
        }
    }
}

/* initializes the glfw window with opengl context and sets up callbacks */
int app_window_init(Application* app, int width, int height) {
    glfwSetErrorCallback(app_window_error_callback);
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    app->window = glfwCreateWindow(width, height, "TinyRequest", NULL, NULL);
    if (!app->window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwSetWindowUserPointer(app->window, app);
    glfwSetWindowCloseCallback(app->window, app_window_close_callback);
    glfwSetKeyCallback(app->window, app_window_key_callback);
    glfwSetWindowSizeLimits(app->window, 600, 400, GLFW_DONT_CARE, GLFW_DONT_CARE);

    glfwMakeContextCurrent(app->window);
    glfwSwapInterval(1);

    return 0;
}

/* cleans up the glfw window and terminates glfw */
void app_window_cleanup(Application* app) {
    if (app->window) {
        glfwDestroyWindow(app->window);
        app->window = NULL;
    }

    glfwTerminate();
}