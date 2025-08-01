/**
 * application core lifecycle management for tinyrequest
 *
 * this file handles the main application initialization, cleanup, and
 * the primary event loop. it coordinates between all the different
 * subsystems - window management, ui rendering, state management,
 * and data persistence.
 *
 * the core manages the application lifecycle from startup through
 * shutdown, ensuring proper initialization order and clean resource
 * cleanup. it also handles the main rendering loop and automatic
 * state synchronization between ui and backend data.
 *
 * window title updates provide visual feedback about the current
 * request status and help users understand what the application
 * is doing at any given moment.
 */

#include "app/app_core.h"
#include "app/app_types.h"
#include "app/app_window.h"
#include "app/app_theme.h"
#include "ui/ui_manager.h"
#include "ui/theme.h"
#include "font_awesome.h"
#include "app_state.h"
#include "persistence.h"
#include <stdio.h>
#include <string.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

/* initializes all application subsystems and prepares for main loop */
int app_core_init(Application* app) {
    printf("TinyRequest HTTP Client starting...\n");
    
    /* initialize glfw and window */
    if (app_window_init(app, WINDOW_WIDTH, WINDOW_HEIGHT) != 0) {
        fprintf(stderr, "Failed to initialize window\n");
        return -1;
    }

    /* apply theming */
    app_theme_apply(app->window);

    /* create application state */
    app->state = app_state_create();
    if (!app->state) {
        fprintf(stderr, "Failed to create application state\n");
        return -1;
    }

    /* create ui manager */
    app->ui_manager = ui_manager_create();
    if (!app->ui_manager) {
        fprintf(stderr, "Failed to create UI manager\n");
        return -1;
    }

    if (ui_manager_init(app->ui_manager, app->window) != 0) {
        fprintf(stderr, "Failed to initialize UI manager\n");
        return -1;
    }

    /* apply theme and initialize font system */
    theme_apply_modern_gruvbox();
    font_awesome_init();

    /* perform initial state synchronization to load any active request data into ui */
    app_state_auto_sync(app->state);

    app->running = true;
    printf("Application initialized successfully\n");
    return 0;
}

/* cleans up all application resources and saves data before shutdown */
void app_core_cleanup(Application* app) {
    if (app->state) {
        if (app->state->collection_manager && app->state->collection_manager->count > 0) {
            int save_result = app_state_save_all_collections(app->state);
            if (save_result == 0) {
                printf("Saved %d collections on shutdown\n", app->state->collection_manager->count);
            } else {
                printf("Warning: Failed to save collections on shutdown\n");
            }
        }
    }

    if (app->ui_manager) {
        ui_manager_cleanup(app->ui_manager);
        ui_manager_destroy(app->ui_manager);
        app->ui_manager = NULL;
    }

    if (app->state) {
        app_state_destroy(app->state);
        app->state = NULL;
    }

    app_window_cleanup(app);
}

/* runs the main application event loop until shutdown */
void app_core_run_main_loop(Application* app) {
    while (!glfwWindowShouldClose(app->window) && app->running) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ui_manager_render(app->ui_manager, app->state);

        if (!app->state->request_in_progress) {
            app_state_auto_sync(app->state);
        }
        
        app_state_check_and_perform_auto_save(app->state);
        app_core_update_window_title(app);

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(app->window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.157f, 0.157f, 0.157f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(app->window);
    }
}

/* updates the window title to reflect current request status */
void app_core_update_window_title(Application* app) {
    if (!app || !app->window || !app->state) {
        return;
    }
    
    char title[512];
    if (app->state->request_in_progress) {
        snprintf(title, sizeof(title), "TinyRequest - Sending %s %s...", 
                app->state->current_request.method, 
                app->state->current_request.url);
    } else if (app->state->current_response.status_code > 0) {
        snprintf(title, sizeof(title), "TinyRequest - %s %s [%d %s]", 
                app->state->current_request.method,
                app->state->current_request.url,
                app->state->current_response.status_code,
                app->state->current_response.status_text);
    } else {
        strcpy(title, "TinyRequest");
    }
    
    glfwSetWindowTitle(app->window, title);
}
