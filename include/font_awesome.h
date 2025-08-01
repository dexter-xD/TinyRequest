/**
 * font_awesome.h
 * 
 * icon system using font awesome for tinyrequest
 * 
 * this file provides all the icons used throughout the application
 * using the font awesome icon font. it includes unicode constants
 * for all the icons we use, plus fallback ascii symbols for when
 * the font isn't available.
 * 
 * covers everything from status indicators and http method icons
 * to ui elements like folders, settings, and performance indicators.
 * also handles loading the font and provides helper functions for
 * rendering icons with text.
 * 
 * makes the ui look professional and gives users visual cues about
 * what different buttons and elements do.
 */

#ifndef FONT_AWESOME_H
#define FONT_AWESOME_H

struct ImVec4;

#ifdef __cplusplus
extern "C" {
#endif

#define ICON_FA_CHECK           "\xef\x80\x8c"
#define ICON_FA_TIMES           "\xef\x80\x8d"
#define ICON_FA_EXCLAMATION     "\xef\x80\x81"
#define ICON_FA_INFO            "\xef\x81\x9a"
#define ICON_FA_SPINNER         "\xef\x81\x90"
#define ICON_FA_CIRCLE_CHECK    "\xef\x81\x98"

#define ICON_FA_COG             "\xef\x80\x93"
#define ICON_FA_CHART_BAR       "\xef\x81\x80"
#define ICON_FA_KEYBOARD        "\xef\x84\x9c"
#define ICON_FA_LIST            "\xef\x80\xba"
#define ICON_FA_FILE            "\xef\x85\x9b"
#define ICON_FA_FILE_TEXT       "\xef\x85\x9c"
#define ICON_FA_CUBE            "\xef\x86\xb2"
#define ICON_FA_GLOBE           "\xef\x82\xac"
#define ICON_FA_LIGHTBULB       "\xef\x83\xab"
#define ICON_FA_TRASH           "\xef\x87\xb8"
#define ICON_FA_SAVE            "\xef\x83\x87"
#define ICON_FA_FOLDER_OPEN     "\xef\x81\xbc"
#define ICON_FA_REFRESH         "\xef\x80\xa1"
#define ICON_FA_XMARK           "\xef\x80\x8d"

#define ICON_FA_FOLDER          "\xef\x81\xbb"
#define ICON_FA_FOLDER_PLUS     "\xef\x99\x9e"
#define ICON_FA_COPY            "\xef\x83\x85"

#define ICON_FA_BOLT            "\xef\x83\xa7"
#define ICON_FA_CLOCK           "\xef\x80\x97"
#define ICON_FA_HOURGLASS       "\xef\x89\x94"

#define ICON_FA_ARROW_RIGHT     "\xef\x81\xa1"
#define ICON_FA_PLUS            "\xef\x81\xa7"
#define ICON_FA_EDIT            "\xef\x81\x84"
#define ICON_FA_MINUS           "\xef\x81\xa8"

#define ICON_FA_CODE            "\xef\x84\xa1"
#define ICON_FA_FILE_CODE       "\xef\x87\x9e"
#define ICON_FA_HTML5           "\xef\x84\xbb"

#define ICON_FA_WIFI            "\xef\x87\xab"
#define ICON_FA_SIGNAL          "\xef\x80\x92"
#define ICON_FA_DOWNLOAD        "\xef\x80\x99"
#define ICON_FA_UPLOAD          "\xef\x80\x93"

#define ICON_FALLBACK_COG             "[*]"
#define ICON_FALLBACK_CHART_BAR       "[#]"
#define ICON_FALLBACK_KEYBOARD        "[K]"
#define ICON_FALLBACK_LIST            "[=]"
#define ICON_FALLBACK_FILE            "[F]"
#define ICON_FALLBACK_FILE_TEXT       "[T]"
#define ICON_FALLBACK_CUBE            "[C]"
#define ICON_FALLBACK_CHECK           "[+]"
#define ICON_FALLBACK_TIMES           "[X]"
#define ICON_FALLBACK_EXCLAMATION     "[!]"
#define ICON_FALLBACK_INFO            "[i]"
#define ICON_FALLBACK_SPINNER         "[~]"
#define ICON_FALLBACK_SAVE            "[S]"
#define ICON_FALLBACK_BOLT            "[>]"
#define ICON_FALLBACK_CLOCK           "[o]"
#define ICON_FALLBACK_HOURGLASS       "[.]"

int font_awesome_init(void);
void font_awesome_cleanup(void);
int font_awesome_is_loaded(void);

void font_awesome_push_icon_font(void);
void font_awesome_pop_icon_font(void);

const char* font_awesome_icon(const char* icon_code);
const char* font_awesome_status_icon(int status_type);
const char* font_awesome_performance_icon(int response_time_ms);
const char* font_awesome_icon_with_fallback(const char* fa_icon, const char* fallback_icon);

void font_awesome_render_icon_text(const char* fa_icon, const char* fallback_icon, const char* text, struct ImVec4 color);

#ifdef __cplusplus
}
#endif

#endif