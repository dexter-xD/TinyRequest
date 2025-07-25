#include <string.h>
#include <math.h>

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Color;

#define BLACK (Color){0, 0, 0, 255}

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

const int FONT_SIZE_NORMAL = 16;
const int FONT_SIZE_SMALL = 12;
const int FONT_SIZE_LARGE = 20;
const int PADDING_SMALL = 4;
const int PADDING_MEDIUM = 8;
const int PADDING_LARGE = 16;

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