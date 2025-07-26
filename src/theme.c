#include "theme.h"
#include <string.h>
#include <math.h>

const int FONT_SIZE_NORMAL = 16;
const int FONT_SIZE_SMALL = 14;
const int FONT_SIZE_LARGE = 20;
const int PADDING_SMALL = 4;
const int PADDING_MEDIUM = 8;
const int PADDING_LARGE = 16;

ThemeFonts theme_fonts = {0};

static Color ColorFromHex(unsigned int hex) {
    return (Color){
        (unsigned char)((hex >> 16) & 0xFF),
        (unsigned char)((hex >> 8) & 0xFF),
        (unsigned char)(hex & 0xFF),
        255
    };
}

GruvboxTheme InitGruvboxTheme(void) {
    GruvboxTheme theme;

    theme.bg0 = ColorFromHex(0x282828);
    theme.bg1 = ColorFromHex(0x3c3836);
    theme.bg2 = ColorFromHex(0x504945);
    theme.fg0 = ColorFromHex(0xfbf1c7);
    theme.fg1 = ColorFromHex(0xebdbb2);
    theme.fg2 = ColorFromHex(0xd5c4a1);
    theme.red = ColorFromHex(0xfb4934);
    theme.green = ColorFromHex(0x98971a);
    theme.yellow = ColorFromHex(0xd79921);
    theme.blue = ColorFromHex(0x458588);
    theme.purple = ColorFromHex(0xb16286);
    theme.aqua = ColorFromHex(0x689d6a);
    theme.orange = ColorFromHex(0xd65d0e);
    theme.gray = ColorFromHex(0x928374);

    return theme;
}

void ApplyThemeToWindow(GruvboxTheme* theme) {
    if (theme == NULL) return;

    ClearBackground(theme->bg0);
}

Color GetStatusColor(int status_code, GruvboxTheme* theme) {
    if (theme == NULL) return BLACK;

    if (status_code >= 100 && status_code < 200) {
        return theme->blue;
    } else if (status_code >= 200 && status_code < 300) {
        return theme->green;
    } else if (status_code >= 300 && status_code < 400) {
        return theme->yellow;
    } else if (status_code >= 400 && status_code < 500) {
        return theme->orange;
    } else if (status_code >= 500 && status_code < 600) {
        return theme->red;
    } else {
        return theme->gray;
    }
}

Color GetSyntaxHighlightColor(const char* token_type, GruvboxTheme* theme) {
    if (theme == NULL || token_type == NULL) return BLACK;

    if (strcmp(token_type, "key") == 0) {
        return theme->blue;
    } else if (strcmp(token_type, "string") == 0) {
        return theme->purple;
    } else if (strcmp(token_type, "number") == 0) {
        return theme->aqua;
    } else if (strcmp(token_type, "boolean") == 0) {
        return theme->orange;
    } else if (strcmp(token_type, "null") == 0) {
        return theme->gray;
    } else if (strcmp(token_type, "punctuation") == 0) {
        return theme->fg1;
    } else if (strcmp(token_type, "error") == 0) {
        return theme->red;
    } else {
        return theme->fg0;
    }
}

Color BlendColors(Color base, Color overlay, float alpha) {

    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    Color result;
    result.r = (unsigned char)(base.r * (1.0f - alpha) + overlay.r * alpha);
    result.g = (unsigned char)(base.g * (1.0f - alpha) + overlay.g * alpha);
    result.b = (unsigned char)(base.b * (1.0f - alpha) + overlay.b * alpha);
    result.a = (unsigned char)(base.a * (1.0f - alpha) + overlay.a * alpha);

    return result;
}

void InitThemeFonts(void) {
    if (theme_fonts.fonts_loaded) return;

    theme_fonts.regular = GetFontDefault();
    theme_fonts.bold = GetFontDefault();
    theme_fonts.monospace = GetFontDefault();
    theme_fonts.title = GetFontDefault();

    #ifdef _WIN32

    Font segoe = LoadFont("C:/Windows/Fonts/segoeui.ttf");
    if (segoe.texture.id != 0) {
        theme_fonts.regular = segoe;
    }

    Font segoe_bold = LoadFont("C:/Windows/Fonts/segoeuib.ttf");
    if (segoe_bold.texture.id != 0) {
        theme_fonts.bold = segoe_bold;
        theme_fonts.title = segoe_bold;
    }

    Font consolas = LoadFont("C:/Windows/Fonts/consola.ttf");
    if (consolas.texture.id != 0) {
        theme_fonts.monospace = consolas;
    }

    if (theme_fonts.bold.texture.id == GetFontDefault().texture.id) {
        Font arial_bold = LoadFont("C:/Windows/Fonts/arialbd.ttf");
        if (arial_bold.texture.id != 0) {
            theme_fonts.bold = arial_bold;
            theme_fonts.title = arial_bold;
        }
    }

    if (theme_fonts.regular.texture.id == GetFontDefault().texture.id) {
        Font calibri = LoadFont("C:/Windows/Fonts/calibri.ttf");
        if (calibri.texture.id != 0) {
            theme_fonts.regular = calibri;
        }
    }

    #endif

    theme_fonts.fonts_loaded = true;
}

void UnloadThemeFonts(void) {
    if (!theme_fonts.fonts_loaded) return;

    if (theme_fonts.regular.texture.id != GetFontDefault().texture.id) {
        UnloadFont(theme_fonts.regular);
    }
    if (theme_fonts.bold.texture.id != GetFontDefault().texture.id) {
        UnloadFont(theme_fonts.bold);
    }
    if (theme_fonts.monospace.texture.id != GetFontDefault().texture.id) {
        UnloadFont(theme_fonts.monospace);
    }
    if (theme_fonts.title.texture.id != GetFontDefault().texture.id) {
        UnloadFont(theme_fonts.title);
    }

    theme_fonts.fonts_loaded = false;
}

Font GetUIFont(void) {
    if (!theme_fonts.fonts_loaded) {
        InitThemeFonts();
    }
    return theme_fonts.regular;
}

Font GetCodeFont(void) {
    if (!theme_fonts.fonts_loaded) {
        InitThemeFonts();
    }
    return theme_fonts.monospace;
}

Font GetTitleFont(void) {
    if (!theme_fonts.fonts_loaded) {
        InitThemeFonts();
    }
    return theme_fonts.title;
}

Font GetBoldFont(void) {
    if (!theme_fonts.fonts_loaded) {
        InitThemeFonts();
    }
    return theme_fonts.bold;
}