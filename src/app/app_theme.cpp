/**
 * application theming and visual styling for tinyrequest
 *
 * this file handles platform-specific window theming including setting
 * the application icon and applying visual styles. on linux systems,
 * most window theming is handled by the desktop environment, but we
 * can still set the window icon and prepare the window for display.
 *
 * the theming system tries multiple icon paths to find the application
 * icon and gracefully falls back if none are found. it works with the
 * imgui theme system to provide a consistent visual experience.
 */

#include "app/app_theme.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h>

/* sets the window icon by trying multiple possible icon locations */
void app_theme_set_window_icon(GLFWwindow* window) {
    if (!window) {
        return;
    }
    
    const char* icon_paths[] = {
        "assets/icon.png",
        "../assets/icon.png",
        "icon.png",
        "/usr/share/icons/hicolor/256x256/apps/tinyrequest.png",
        nullptr
    };
    
    for (int i = 0; icon_paths[i] != nullptr; i++) {
        int width, height, channels;
        unsigned char* image_data = stbi_load(icon_paths[i], &width, &height, &channels, 4);
        
        if (image_data) {
            GLFWimage icon;
            icon.width = width;
            icon.height = height;
            icon.pixels = image_data;
            
            glfwSetWindowIcon(window, 1, &icon);
            
            stbi_image_free(image_data);
            printf("Window icon set from: %s\n", icon_paths[i]);
            return;
        }
    }
    
    printf("Failed to load window icon\n");
}

/* applies platform-specific window theming (limited on linux) */
void app_theme_set_window_theme(GLFWwindow* window) {
    (void)window;
    printf("Window theming handled by desktop environment on Linux\n");
}

/* applies all available theming options and shows the window */
void app_theme_apply(GLFWwindow* window) {
    app_theme_set_window_icon(window);
    app_theme_set_window_theme(window);
    
    glfwShowWindow(window);
}