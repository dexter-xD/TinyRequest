/**
 * font awesome icon definitions for tinyrequest
 *
 * this file provides the complete set of font awesome icon definitions
 * used throughout the tinyrequest user interface. it includes all the
 * unicode codepoints for the icons in a convenient format for use with
 * imgui and other ui frameworks.
 *
 * the icons are organized by category and provide visual elements for
 * buttons, status indicators, and other ui components to make the
 * interface more intuitive and visually appealing.
 */

#include "font_awesome.h"
#include "imgui.h"
#include <stdio.h>
#include <stdlib.h>

extern "C" {

static bool g_font_awesome_initialized = false;
static bool g_font_awesome_font_loaded = false;
static ImFont* g_font_awesome_font = nullptr;

int font_awesome_init(void) {
    ImGuiIO& io = ImGui::GetIO();

    if (io.Fonts->Fonts.Size == 0) {
        io.Fonts->AddFontDefault();
    }

    const char* font_paths[] = {
        "assets/fonts/fa-solid-900.ttf",           
        "../assets/fonts/fa-solid-900.ttf",       
        "fonts/fa-solid-900.ttf",                  
        "/usr/share/tinyrequest/assets/fonts/fa-solid-900.ttf", 
        "/usr/share/fonts/truetype/font-awesome/fa-solid-900.ttf", 
        "/usr/share/fonts/fontawesome/fa-solid-900.ttf", 
        nullptr
    };

    ImFontConfig config;
    config.MergeMode = true;  
    config.PixelSnapH = true;
    config.GlyphMinAdvanceX = 13.0f;

    static const ImWchar icon_ranges[] = {
        0xf000, 0xf3ff, 
        0,
    };

    for (int i = 0; font_paths[i] != nullptr; i++) {
        FILE* font_file = fopen(font_paths[i], "rb");
        if (font_file) {
            fclose(font_file);
            g_font_awesome_font = io.Fonts->AddFontFromFileTTF(font_paths[i], 16.0f, &config, icon_ranges);
            if (g_font_awesome_font) {
                g_font_awesome_font_loaded = true;
                printf("Font Awesome loaded from: %s\n", font_paths[i]);
                break;
            }
        }
    }

    io.Fonts->Build();

    if (g_font_awesome_font_loaded) {
        printf("Font Awesome icon system initialized with TTF font\n");
    } else {
        printf("Font Awesome icon system initialized with Unicode symbols (no TTF font found)\n");
    }

    g_font_awesome_initialized = true;
    return 0;
}

void font_awesome_cleanup(void) {
    g_font_awesome_initialized = false;
    g_font_awesome_font_loaded = false;
    g_font_awesome_font = nullptr;
}

int font_awesome_is_loaded(void) {
    return g_font_awesome_font_loaded ? 1 : 0;
}

void font_awesome_push_icon_font(void) {
    if (g_font_awesome_font_loaded && g_font_awesome_font) {
        ImGui::PushFont(g_font_awesome_font);
    }

}

void font_awesome_pop_icon_font(void) {
    if (g_font_awesome_font_loaded && g_font_awesome_font) {
        ImGui::PopFont();
    }

}

void font_awesome_render_icon(const char* icon, ImVec4 color) {
    if (!icon) return;

    if (g_font_awesome_font_loaded) {
        font_awesome_push_icon_font();
        ImGui::TextColored(color, "%s", icon);
        font_awesome_pop_icon_font();
    } else {
        ImGui::TextColored(color, "%s", icon);
    }
}

const char* font_awesome_get_status_icon(int status_type) {
    switch (status_type) {
        case 0: return ICON_FA_CIRCLE_CHECK;  
        case 1: return ICON_FA_EXCLAMATION;   
        case 2: return ICON_FA_TIMES;         
        case 3: return ICON_FA_INFO;          
        case 4: return ICON_FA_SPINNER;       
        default: return ICON_FA_INFO;
    }
}

const char* font_awesome_get_performance_icon(int response_time_ms) {
    if (response_time_ms < 100) {
        return ICON_FA_BOLT;        
    } else if (response_time_ms < 1000) {
        return ICON_FA_CLOCK;       
    } else {
        return ICON_FA_HOURGLASS;   
    }
}

const char* font_awesome_icon(const char* icon_code) {
    return icon_code;
}

const char* font_awesome_status_icon(int status_type) {
    if (g_font_awesome_font_loaded) {

        return font_awesome_get_status_icon(status_type);
    } else {

        switch (status_type) {
            case 0: return ICON_FALLBACK_CHECK;       
            case 1: return ICON_FALLBACK_EXCLAMATION; 
            case 2: return ICON_FALLBACK_TIMES;       
            case 3: return ICON_FALLBACK_INFO;        
            case 4: return ICON_FALLBACK_SPINNER;     
            default: return ICON_FALLBACK_INFO;
        }
    }
}

const char* font_awesome_performance_icon(int response_time_ms) {
    if (g_font_awesome_font_loaded) {

        return font_awesome_get_performance_icon(response_time_ms);
    } else {

        if (response_time_ms < 100) {
            return ICON_FALLBACK_BOLT;        
        } else if (response_time_ms < 1000) {
            return ICON_FALLBACK_CLOCK;       
        } else {
            return ICON_FALLBACK_HOURGLASS;   
        }
    }
}

const char* font_awesome_icon_with_fallback(const char* fa_icon, const char* fallback_icon) {
    if (g_font_awesome_font_loaded) {
        return fa_icon;
    } else {
        return fallback_icon;
    }
}

void font_awesome_render_icon_text(const char* fa_icon, const char* fallback_icon, const char* text, struct ImVec4 color) {

    const char* icon = font_awesome_icon_with_fallback(fa_icon, fallback_icon);

    ImVec2 cursor_pos = ImGui::GetCursorPos();

    if (g_font_awesome_font_loaded) {
        font_awesome_push_icon_font();
        ImGui::TextColored(color, "%s", icon);
        font_awesome_pop_icon_font();
    } else {
        ImGui::TextColored(color, "%s", icon);
    }

    ImVec2 icon_size = ImGui::CalcTextSize(icon);
    ImGui::SameLine();
    ImGui::SetCursorPosX(cursor_pos.x + icon_size.x + 4.0f); 

    ImGui::TextColored(color, "%s", text);
}

} 