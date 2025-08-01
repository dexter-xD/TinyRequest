#include "ui/theme.h"
#include "font_awesome.h"
#include <stdlib.h>
#include <math.h>

extern "C" {

/* Global theme instance */ 
static ModernGruvboxTheme g_theme;
static bool g_theme_initialized = false;

void theme_init_modern_gruvbox(ModernGruvboxTheme* theme) {
    if (!theme) {
        return;
    }
    
    /* Background variants - Gruvbox Dark palette */
    theme->bg_hard = theme_hex_to_imvec4(0x1d2021);      // Hard background
    theme->bg_normal = theme_hex_to_imvec4(0x282828);    // Normal background
    theme->bg_soft = theme_hex_to_imvec4(0x32302f);      // Soft background
    theme->bg_panel = theme_hex_to_imvec4(0x3c3836);     // Panel background
    theme->bg_elevated = theme_hex_to_imvec4(0x504945);  // Elevated elements
    theme->bg_input = theme_hex_to_imvec4(0x3c3836);     // Input fields
    theme->bg_hover = theme_hex_to_imvec4(0x504945);     // Hover states
    
    /* Foreground variants */
    theme->fg_primary = theme_hex_to_imvec4(0xfbf1c7);   // Primary text
    theme->fg_secondary = theme_hex_to_imvec4(0xebdbb2); // Secondary text
    theme->fg_tertiary = theme_hex_to_imvec4(0xd5c4a1);  // Tertiary text
    theme->fg_disabled = theme_hex_to_imvec4(0xa89984);  // Disabled text
    
    /*  Semantic colors */
    theme->success = theme_hex_to_imvec4(0xb8bb26);      // Green
    theme->warning = theme_hex_to_imvec4(0xfabd2f);      // Yellow
    theme->error = theme_hex_to_imvec4(0xfb4934);        // Red
    theme->info = theme_hex_to_imvec4(0x83a598);         // Blue
    theme->accent_primary = theme_hex_to_imvec4(0xfe8019);   // Orange
    theme->accent_secondary = theme_hex_to_imvec4(0xd3869b); // Purple
    
    /*  Interactive button states */
    theme->button_normal = theme_hex_to_imvec4(0x504945);    // Normal button
    theme->button_hovered = theme_hex_to_imvec4(0x665c54);  // Hovered button
    theme->button_active = theme_hex_to_imvec4(0x7c6f64);   // Active button
    theme->button_primary = theme_hex_to_imvec4(0xfe8019);  // Primary button
    theme->button_success = theme_hex_to_imvec4(0xb8bb26);  // Success button
    theme->button_danger = theme_hex_to_imvec4(0xfb4934);   // Danger button
    
    /*  Borders and separators */
    theme->border_normal = theme_hex_to_imvec4(0x665c54);   // Normal borders
    theme->border_focus = theme_hex_to_imvec4(0xfe8019);    // Focused borders
    theme->separator = theme_hex_to_imvec4(0x504945);       // Separators
    theme->shadow = theme_hex_to_imvec4(0x1d2021);          // Shadows
    
    /* Status indicators */
    theme->status_success = theme_hex_to_imvec4(0xb8bb26);  // Success status
    theme->status_warning = theme_hex_to_imvec4(0xfabd2f);  // Warning status
    theme->status_error = theme_hex_to_imvec4(0xfb4934);    // Error status
    theme->status_info = theme_hex_to_imvec4(0x83a598);     // Info status
    theme->status_loading = theme_hex_to_imvec4(0xd3869b);  // Loading status
}

void theme_apply_modern_gruvbox(void) {
    theme_init_modern_gruvbox(&g_theme);
    theme_apply_colors(&g_theme);
    theme_configure_style();
    g_theme_initialized = true;
}

void theme_apply_colors(const ModernGruvboxTheme* theme) {
    if (!theme) {
        return;
    }

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = theme->bg_normal;
    colors[ImGuiCol_ChildBg] = theme->bg_panel;
    colors[ImGuiCol_PopupBg] = theme->bg_elevated;
    colors[ImGuiCol_MenuBarBg] = theme->bg_panel;

    colors[ImGuiCol_Text] = theme->fg_primary;
    colors[ImGuiCol_TextDisabled] = theme->fg_disabled;
    colors[ImGuiCol_TextSelectedBg] = theme_alpha_blend(theme->accent_primary, 0.3f);

    colors[ImGuiCol_Border] = theme->border_normal;
    colors[ImGuiCol_BorderShadow] = theme_alpha_blend(theme->shadow, 0.0f);

    colors[ImGuiCol_FrameBg] = theme->bg_input;
    colors[ImGuiCol_FrameBgHovered] = theme->bg_hover;
    colors[ImGuiCol_FrameBgActive] = theme_lighten_color(theme->bg_hover, 0.2f);

    colors[ImGuiCol_TitleBg] = theme->bg_elevated;
    colors[ImGuiCol_TitleBgActive] = theme_alpha_blend(theme->accent_primary, 0.8f);
    colors[ImGuiCol_TitleBgCollapsed] = theme->bg_panel;

    colors[ImGuiCol_ScrollbarBg] = theme->bg_normal;
    colors[ImGuiCol_ScrollbarGrab] = theme->button_normal;
    colors[ImGuiCol_ScrollbarGrabHovered] = theme->button_hovered;
    colors[ImGuiCol_ScrollbarGrabActive] = theme->button_active;

    colors[ImGuiCol_CheckMark] = theme->accent_primary;
    colors[ImGuiCol_SliderGrab] = theme->accent_primary;
    colors[ImGuiCol_SliderGrabActive] = theme_lighten_color(theme->accent_primary, 0.2f);

    colors[ImGuiCol_Button] = theme->button_normal;
    colors[ImGuiCol_ButtonHovered] = theme->button_hovered;
    colors[ImGuiCol_ButtonActive] = theme->button_active;

    colors[ImGuiCol_Header] = theme->bg_elevated;
    colors[ImGuiCol_HeaderHovered] = theme->bg_hover;
    colors[ImGuiCol_HeaderActive] = theme->button_active;

    colors[ImGuiCol_Separator] = theme->separator;
    colors[ImGuiCol_SeparatorHovered] = theme->accent_primary;
    colors[ImGuiCol_SeparatorActive] = theme_lighten_color(theme->accent_primary, 0.2f);

    colors[ImGuiCol_ResizeGrip] = theme->button_normal;
    colors[ImGuiCol_ResizeGripHovered] = theme->button_hovered;
    colors[ImGuiCol_ResizeGripActive] = theme->button_active;

    colors[ImGuiCol_Tab] = theme->bg_panel;
    colors[ImGuiCol_TabHovered] = theme->bg_hover;
    colors[ImGuiCol_TabActive] = theme->bg_elevated;
    colors[ImGuiCol_TabUnfocused] = theme->bg_panel;
    colors[ImGuiCol_TabUnfocusedActive] = theme->bg_elevated;

    colors[ImGuiCol_PlotLines] = theme->accent_primary;
    colors[ImGuiCol_PlotLinesHovered] = theme->success;
    colors[ImGuiCol_PlotHistogram] = theme->accent_primary;
    colors[ImGuiCol_PlotHistogramHovered] = theme->success;

    colors[ImGuiCol_TableHeaderBg] = theme->bg_elevated;
    colors[ImGuiCol_TableBorderStrong] = theme->border_normal;
    colors[ImGuiCol_TableBorderLight] = theme_alpha_blend(theme->border_normal, 0.5f);
    colors[ImGuiCol_TableRowBg] = theme_alpha_blend(theme->bg_normal, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = theme_alpha_blend(theme->bg_panel, 0.3f);

    colors[ImGuiCol_DragDropTarget] = theme->accent_primary;

    colors[ImGuiCol_NavHighlight] = theme->accent_primary;
    colors[ImGuiCol_NavWindowingHighlight] = theme->accent_primary;
    colors[ImGuiCol_NavWindowingDimBg] = theme_alpha_blend(theme->bg_hard, 0.8f);
    colors[ImGuiCol_ModalWindowDimBg] = theme_alpha_blend(theme->bg_hard, 0.8f);
}

void theme_configure_style(void) {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 0.0f;
    style.FrameRounding = RADIUS_SM;
    style.GrabRounding = RADIUS_SM;
    style.ScrollbarRounding = RADIUS_SM;
    style.TabRounding = RADIUS_SM;
    style.ChildRounding = RADIUS_SM;
    style.PopupRounding = RADIUS_MD;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.TabBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;

    style.ScrollbarSize = 16.0f;
    style.GrabMinSize = 12.0f;

    style.WindowPadding = ImVec2(SPACING_MD, SPACING_MD);
    style.FramePadding = ImVec2(SPACING_SM, SPACING_SM);
    style.ItemSpacing = ImVec2(SPACING_SM, SPACING_SM);
    style.ItemInnerSpacing = ImVec2(SPACING_XS, SPACING_XS);
    style.IndentSpacing = SPACING_LG;
    style.ColumnsMinSpacing = SPACING_SM;

    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.WindowMenuButtonPosition = ImGuiDir_None;
}

void theme_push_button_style(const ModernGruvboxTheme* theme, int button_type) {
    if (!theme) return;

    ImVec4 normal, hovered, active;

    switch (button_type) {
        case BUTTON_TYPE_PRIMARY:
            normal = theme->button_primary;
            hovered = theme_lighten_color(theme->button_primary, 0.1f);
            active = theme_darken_color(theme->button_primary, 0.1f);
            break;
        case BUTTON_TYPE_SUCCESS:
            normal = theme->button_success;
            hovered = theme_lighten_color(theme->button_success, 0.1f);
            active = theme_darken_color(theme->button_success, 0.1f);
            break;
        case BUTTON_TYPE_DANGER:
            normal = theme->button_danger;
            hovered = theme_lighten_color(theme->button_danger, 0.1f);
            active = theme_darken_color(theme->button_danger, 0.1f);
            break;
        default: 
            normal = theme->button_normal;
            hovered = theme->button_hovered;
            active = theme->button_active;
            break;
    }

    ImGui::PushStyleColor(ImGuiCol_Button, normal);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, active);
}

void theme_pop_button_style(void) {
    ImGui::PopStyleColor(3);
}

void theme_push_input_style(const ModernGruvboxTheme* theme) {
    if (!theme) return;

    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme->bg_input);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme->bg_hover);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, theme_lighten_color(theme->bg_hover, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_Border, theme->border_normal);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(SPACING_SM, SPACING_SM));
}

void theme_pop_input_style(void) {
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(1);
}

void theme_push_panel_style(const ModernGruvboxTheme* theme) {
    if (!theme) return;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme->bg_panel);
    ImGui::PushStyleColor(ImGuiCol_Border, theme->border_normal);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, RADIUS_MD);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(SPACING_MD, SPACING_MD));
}

void theme_pop_panel_style(void) {
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void theme_push_header_style(void) {
    ImGui::PushFont(nullptr); 

}

void theme_push_body_style(void) {
    ImGui::PushFont(nullptr); 
}

void theme_push_caption_style(void) {
    ImGui::PushFont(nullptr); 

}

void theme_push_code_style(void) {
    ImGui::PushFont(nullptr); 

}

void theme_pop_text_style(void) {
    ImGui::PopFont();
}

void theme_render_status_indicator(const char* text, int status_type, const ModernGruvboxTheme* theme) {
    if (!text || !theme) return;

    ImVec4 color;
    const char* icon = "";

    switch (status_type) {
        case STATUS_TYPE_SUCCESS:
            color = theme->status_success;
            icon = font_awesome_status_icon(0);
            break;
        case STATUS_TYPE_WARNING:
            color = theme->status_warning;
            icon = font_awesome_status_icon(1);
            break;
        case STATUS_TYPE_ERROR:
            color = theme->status_error;
            icon = font_awesome_status_icon(2);
            break;
        case STATUS_TYPE_INFO:
            color = theme->status_info;
            icon = font_awesome_status_icon(3);
            break;
        case STATUS_TYPE_LOADING:
            color = theme->status_loading;
            icon = font_awesome_status_icon(4);
            break;
        default:
            color = theme->fg_secondary;
            icon = "";
            break;
    }

    ImGui::TextColored(color, "%s %s", icon, text);
}

void theme_render_loading_spinner(const ModernGruvboxTheme* theme) {
    if (!theme) return;

    static float spinner_time = 0.0f;
    spinner_time += ImGui::GetIO().DeltaTime;

    const char* spinner_chars[] = {"|", "/", "-", "\\", "|", "/", "-", "\\"};
    int spinner_index = (int)(spinner_time * 4.0f) % 8;

    ImGui::TextColored(theme->status_loading, "%s", spinner_chars[spinner_index]);
}

ImVec4 theme_hex_to_imvec4(unsigned int hex) {
    float r = ((hex >> 16) & 0xFF) / 255.0f;
    float g = ((hex >> 8) & 0xFF) / 255.0f;
    float b = (hex & 0xFF) / 255.0f;
    return ImVec4(r, g, b, 1.0f);
}

ImVec4 theme_rgb_to_imvec4(int r, int g, int b, int a) {
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

ImVec4 theme_lighten_color(ImVec4 color, float factor) {
    return ImVec4(
        fminf(color.x + factor, 1.0f),
        fminf(color.y + factor, 1.0f),
        fminf(color.z + factor, 1.0f),
        color.w
    );
}

ImVec4 theme_darken_color(ImVec4 color, float factor) {
    return ImVec4(
        fmaxf(color.x - factor, 0.0f),
        fmaxf(color.y - factor, 0.0f),
        fmaxf(color.z - factor, 0.0f),
        color.w
    );
}

ImVec4 theme_alpha_blend(ImVec4 color, float alpha) {
    return ImVec4(color.x, color.y, color.z, alpha);
}

const ModernGruvboxTheme* theme_get_current(void) {
    if (!g_theme_initialized) {
        theme_apply_modern_gruvbox();
    }
    return &g_theme;
}

void theme_init(void) {
    theme_apply_modern_gruvbox();
}

void theme_cleanup(void) {

    g_theme_initialized = false;
}

void theme_push_button_style_c(int button_type) {
    const ModernGruvboxTheme* theme = theme_get_current();
    theme_push_button_style(theme, button_type);
}

void theme_push_input_style_c(void) {
    const ModernGruvboxTheme* theme = theme_get_current();
    theme_push_input_style(theme);
}

void theme_push_panel_style_c(void) {
    const ModernGruvboxTheme* theme = theme_get_current();
    theme_push_panel_style(theme);
}

} 