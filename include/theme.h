#ifndef THEME_H
#define THEME_H

#include "raylib.h"

typedef struct {
    Color bg0;
    Color bg1;
    Color bg2;
    Color fg0;
    Color fg1;
    Color fg2;
    Color red;
    Color green;
    Color yellow;
    Color blue;
    Color purple;
    Color aqua;
    Color orange;
    Color gray;
} GruvboxTheme;

GruvboxTheme InitGruvboxTheme(void);
void ApplyThemeToWindow(GruvboxTheme* theme);

Color GetStatusColor(int status_code, GruvboxTheme* theme);
Color GetSyntaxHighlightColor(const char* token_type, GruvboxTheme* theme);
Color BlendColors(Color base, Color overlay, float alpha);

extern const int FONT_SIZE_NORMAL;
extern const int FONT_SIZE_SMALL;
extern const int FONT_SIZE_LARGE;
extern const int PADDING_SMALL;
extern const int PADDING_MEDIUM;
extern const int PADDING_LARGE;

typedef struct {
    Font regular;
    Font bold;
    Font monospace;
    Font title;
    bool fonts_loaded;
} ThemeFonts;

extern ThemeFonts theme_fonts;

void InitThemeFonts(void);
void UnloadThemeFonts(void);
Font GetUIFont(void);
Font GetCodeFont(void);
Font GetTitleFont(void);
Font GetBoldFont(void);

#endif