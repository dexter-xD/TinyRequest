#ifndef UI_STYLE_H
#define UI_STYLE_H

#include "theme.h"
#include "raylib.h"

void DrawRoundedRect(Rectangle bounds, float radius, Color color);

void DrawRoundedRectLines(Rectangle bounds, float radius, Color color);

void DrawRectShadow(Rectangle bounds, float blur_radius, Color shadow_color);

void DrawPanelCard(Rectangle bounds, const GruvboxTheme* theme);

Color GruvboxPanelBg(const GruvboxTheme* theme);
Color GruvboxPanelBorder(const GruvboxTheme* theme);
Color GruvboxButtonBg(const GruvboxTheme* theme, bool hovered, bool active);
Color GruvboxButtonBorder(const GruvboxTheme* theme, bool hovered, bool active);

void DrawButtonWithIcon(Rectangle bounds, const char* text, bool pressed, GruvboxTheme* theme, void (*DrawIcon)(Vector2 center, float size, Color color));

#endif