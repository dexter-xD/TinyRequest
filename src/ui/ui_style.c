#include "ui_style.h"

void DrawRoundedRect(Rectangle bounds, float radius, Color color) {
    DrawRectangleRounded(bounds, radius / (bounds.height / 2.0f), 16, color);
}

void DrawRoundedRectLines(Rectangle bounds, float radius, Color color) {
    DrawRectangleRoundedLines(bounds, radius / (bounds.height / 2.0f), 16, color);
}

void DrawRectShadow(Rectangle bounds, float blur_radius, Color shadow_color) {
    Rectangle shadow_bounds = bounds;
    shadow_bounds.x += blur_radius;
    shadow_bounds.y += blur_radius;
    DrawRectangleRounded(shadow_bounds, blur_radius / (bounds.height / 2.0f), 16, Fade(shadow_color, 0.3f));
}

Color GruvboxPanelBg(const GruvboxTheme* theme) {
    return BlendColors(theme->bg1, theme->fg0, 0.06f);
}

Color GruvboxPanelBorder(const GruvboxTheme* theme) {
    return theme->gray;
}

Color GruvboxButtonBg(const GruvboxTheme* theme, bool hovered, bool active) {
    if (active) return theme->green;
    if (hovered) return BlendColors(theme->bg2, theme->green, 0.18f);
    return BlendColors(theme->bg1, theme->green, 0.10f);
}

Color GruvboxButtonBorder(const GruvboxTheme* theme, bool hovered, bool active) {
    if (active) return theme->green;
    if (hovered) return theme->yellow;
    return theme->gray;
}

void DrawPanelCard(Rectangle bounds, const GruvboxTheme* theme) {
    float radius = 12.0f;
    DrawRectShadow(bounds, 8.0f, theme->bg0);
    DrawRoundedRect(bounds, radius, GruvboxPanelBg(theme));
    DrawRoundedRectLines(bounds, radius, GruvboxPanelBorder(theme));
}

void DrawButtonWithIcon(Rectangle bounds, const char* text, bool pressed, GruvboxTheme* theme, void (*DrawIcon)(Vector2 center, float size, Color color)) {
    if (!text || !theme) return;
    float radius = 8.0f;
    DrawRectShadow(bounds, 6.0f, theme->bg0);
    Color bg_color = GruvboxButtonBg(theme, pressed, pressed);
    Color border_color = GruvboxButtonBorder(theme, pressed, pressed);
    DrawRoundedRect(bounds, radius, bg_color);
    DrawRoundedRectLines(bounds, radius, border_color);
    float icon_space = DrawIcon ? bounds.height : 0;
    Vector2 text_size = MeasureTextEx(GetFontDefault(), text, FONT_SIZE_NORMAL, 1);
    float total_width = text_size.x + (DrawIcon ? icon_space + 6 : 0);
    float text_x = bounds.x + (bounds.width - total_width) / 2 + (DrawIcon ? icon_space + 6 : 0);
    float text_y = bounds.y + (bounds.height - text_size.y) / 2;
    if (DrawIcon) {
        Vector2 icon_center = {bounds.x + (bounds.width - total_width) / 2 + icon_space / 2, bounds.y + bounds.height / 2};
        DrawIcon(icon_center, bounds.height * 0.55f, theme->fg0);
    }
    DrawTextEx(GetFontDefault(), text, (Vector2){text_x, text_y}, FONT_SIZE_NORMAL, 1, theme->fg0);
}