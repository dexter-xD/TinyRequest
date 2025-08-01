/**
 * theme.h
 * 
 * visual theming and color system for tinyrequest
 * 
 * this file defines the complete visual design system for the application
 * using a modern gruvbox color palette. it includes all the colors for
 * backgrounds, text, buttons, status indicators, and interactive elements.
 * 
 * provides functions for applying consistent styling across all ui components,
 * managing different button types and states, and utility functions for
 * color manipulation. also includes constants for spacing, typography,
 * and border radius to maintain visual consistency.
 * 
 * basically everything needed to make the application look professional
 * and cohesive throughout all its interfaces.
 */

#ifndef THEME_H
#define THEME_H

#ifdef __cplusplus
#include "imgui.h"
#else
typedef struct ImVec4 {
    float x, y, z, w;
} ImVec4;
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ImVec4 bg_hard;
    ImVec4 bg_normal;
    ImVec4 bg_soft;
    ImVec4 bg_panel;
    ImVec4 bg_elevated;
    ImVec4 bg_input;
    ImVec4 bg_hover;
    
    ImVec4 fg_primary;
    ImVec4 fg_secondary;
    ImVec4 fg_tertiary;
    ImVec4 fg_disabled;
    
    ImVec4 success;
    ImVec4 warning;
    ImVec4 error;
    ImVec4 info;
    ImVec4 accent_primary;
    ImVec4 accent_secondary;
    
    ImVec4 button_normal;
    ImVec4 button_hovered;
    ImVec4 button_active;
    ImVec4 button_primary;
    ImVec4 button_success;
    ImVec4 button_danger;
    
    ImVec4 border_normal;
    ImVec4 border_focus;
    ImVec4 separator;
    ImVec4 shadow;
    
    ImVec4 status_success;
    ImVec4 status_warning;
    ImVec4 status_error;
    ImVec4 status_info;
    ImVec4 status_loading;
} ModernGruvboxTheme;

#define SPACING_XS  4.0f
#define SPACING_SM  8.0f
#define SPACING_MD  16.0f
#define SPACING_LG  24.0f
#define SPACING_XL  32.0f

#define RADIUS_SM   4.0f
#define RADIUS_MD   6.0f
#define RADIUS_LG   8.0f
#define RADIUS_XL   12.0f

#define FONT_SIZE_SM    12.0f
#define FONT_SIZE_MD    14.0f
#define FONT_SIZE_LG    16.0f
#define FONT_SIZE_XL    18.0f

#define BUTTON_TYPE_NORMAL    0
#define BUTTON_TYPE_PRIMARY   1
#define BUTTON_TYPE_SUCCESS   2
#define BUTTON_TYPE_DANGER    3

#define STATUS_TYPE_SUCCESS   0
#define STATUS_TYPE_WARNING   1
#define STATUS_TYPE_ERROR     2
#define STATUS_TYPE_INFO      3
#define STATUS_TYPE_LOADING   4

void theme_init_modern_gruvbox(ModernGruvboxTheme* theme);
void theme_apply_modern_gruvbox(void);
void theme_apply_colors(const ModernGruvboxTheme* theme);
void theme_configure_style(void);

void theme_push_button_style(const ModernGruvboxTheme* theme, int button_type);
void theme_pop_button_style(void);
void theme_push_input_style(const ModernGruvboxTheme* theme);
void theme_pop_input_style(void);
void theme_push_panel_style(const ModernGruvboxTheme* theme);
void theme_pop_panel_style(void);

void theme_push_header_style(void);
void theme_push_body_style(void);
void theme_push_caption_style(void);
void theme_push_code_style(void);
void theme_pop_text_style(void);

void theme_render_status_indicator(const char* text, int status_type, const ModernGruvboxTheme* theme);
void theme_render_loading_spinner(const ModernGruvboxTheme* theme);

ImVec4 theme_hex_to_imvec4(unsigned int hex);
ImVec4 theme_rgb_to_imvec4(int r, int g, int b, int a);
ImVec4 theme_lighten_color(ImVec4 color, float factor);
ImVec4 theme_darken_color(ImVec4 color, float factor);
ImVec4 theme_alpha_blend(ImVec4 color, float alpha);

const ModernGruvboxTheme* theme_get_current(void);

void theme_init(void);
void theme_cleanup(void);
void theme_push_button_style_c(int button_type);
void theme_push_input_style_c(void);
void theme_push_panel_style_c(void);

#ifdef __cplusplus
}
#endif

#endif