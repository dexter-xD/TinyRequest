/**
 * app_theme.h
 * 
 * visual theming and styling for tinyrequest
 * 
 * this file handles making the application look good - setting up
 * the color scheme, fonts, window decorations, and all the visual
 * polish that makes the difference between a functional tool and
 * something people actually want to use.
 * 
 * includes setting the window icon and applying consistent styling
 * across all the ui elements.
 */

#ifndef APP_THEME_H
#define APP_THEME_H

#include <GLFW/glfw3.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_theme_apply(GLFWwindow* window);
void app_theme_set_window_icon(GLFWwindow* window);
void app_theme_set_window_theme(GLFWwindow* window);

#ifdef __cplusplus
}
#endif

#endif