#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "theme_test_impl.c"

void test_color_equality(Color expected, Color actual, const char* test_name) {
    if (expected.r != actual.r || expected.g != actual.g ||
        expected.b != actual.b || expected.a != actual.a) {
        printf("FAIL: %s - Expected RGBA(%d,%d,%d,%d), got RGBA(%d,%d,%d,%d)\n",
               test_name, expected.r, expected.g, expected.b, expected.a,
               actual.r, actual.g, actual.b, actual.a);
        assert(0);
    } else {
        printf("PASS: %s\n", test_name);
    }
}

double calculate_luminance(Color color) {
    double r = color.r / 255.0;
    double g = color.g / 255.0;
    double b = color.b / 255.0;

    r = (r <= 0.03928) ? r / 12.92 : pow((r + 0.055) / 1.055, 2.4);
    g = (g <= 0.03928) ? g / 12.92 : pow((g + 0.055) / 1.055, 2.4);
    b = (b <= 0.03928) ? b / 12.92 : pow((b + 0.055) / 1.055, 2.4);

    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

double calculate_contrast_ratio(Color color1, Color color2) {
    double lum1 = calculate_luminance(color1);
    double lum2 = calculate_luminance(color2);

    double lighter = (lum1 > lum2) ? lum1 : lum2;
    double darker = (lum1 > lum2) ? lum2 : lum1;

    return (lighter + 0.05) / (darker + 0.05);
}

void test_theme_initialization() {
    printf("\n=== Testing Theme Initialization ===\n");

    GruvboxTheme theme = InitGruvboxTheme();

    assert(theme.bg0.r != 0 || theme.bg0.g != 0 || theme.bg0.b != 0);
    assert(theme.fg0.r != 0 || theme.fg0.g != 0 || theme.fg0.b != 0);

    Color expected_bg0 = {0x28, 0x28, 0x28, 255};
    test_color_equality(expected_bg0, theme.bg0, "Background color (bg0)");

    Color expected_fg0 = {0xfb, 0xf1, 0xc7, 255};
    test_color_equality(expected_fg0, theme.fg0, "Foreground color (fg0)");

    Color expected_red = {0xfb, 0x49, 0x34, 255};
    test_color_equality(expected_red, theme.red, "Red color");

    Color expected_green = {0x98, 0x97, 0x1a, 255};
    test_color_equality(expected_green, theme.green, "Green color");
}

void test_status_colors() {
    printf("\n=== Testing HTTP Status Colors ===\n");

    GruvboxTheme theme = InitGruvboxTheme();

    Color success_color = GetStatusColor(200, &theme);
    test_color_equality(theme.green, success_color, "HTTP 200 status color");

    Color created_color = GetStatusColor(201, &theme);
    test_color_equality(theme.green, created_color, "HTTP 201 status color");

    Color redirect_color = GetStatusColor(301, &theme);
    test_color_equality(theme.yellow, redirect_color, "HTTP 301 status color");

    Color not_found_color = GetStatusColor(404, &theme);
    test_color_equality(theme.orange, not_found_color, "HTTP 404 status color");

    Color server_error_color = GetStatusColor(500, &theme);
    test_color_equality(theme.red, server_error_color, "HTTP 500 status color");

    Color info_color = GetStatusColor(100, &theme);
    test_color_equality(theme.blue, info_color, "HTTP 100 status color");

    Color invalid_color = GetStatusColor(999, &theme);
    test_color_equality(theme.gray, invalid_color, "Invalid status code color");

    Color null_result = GetStatusColor(200, NULL);
    test_color_equality(BLACK, null_result, "NULL theme handling");
}

void test_syntax_highlighting() {
    printf("\n=== Testing JSON Syntax Highlighting ===\n");

    GruvboxTheme theme = InitGruvboxTheme();

    Color key_color = GetSyntaxHighlightColor("key", &theme);
    test_color_equality(theme.blue, key_color, "JSON key color");

    Color string_color = GetSyntaxHighlightColor("string", &theme);
    test_color_equality(theme.purple, string_color, "JSON string color");

    Color number_color = GetSyntaxHighlightColor("number", &theme);
    test_color_equality(theme.aqua, number_color, "JSON number color");

    Color boolean_color = GetSyntaxHighlightColor("boolean", &theme);
    test_color_equality(theme.orange, boolean_color, "JSON boolean color");

    Color null_color = GetSyntaxHighlightColor("null", &theme);
    test_color_equality(theme.gray, null_color, "JSON null color");

    Color punct_color = GetSyntaxHighlightColor("punctuation", &theme);
    test_color_equality(theme.fg1, punct_color, "JSON punctuation color");

    Color error_color = GetSyntaxHighlightColor("error", &theme);
    test_color_equality(theme.red, error_color, "JSON error color");

    Color unknown_color = GetSyntaxHighlightColor("unknown", &theme);
    test_color_equality(theme.fg0, unknown_color, "Unknown token type color");

    Color null_token_result = GetSyntaxHighlightColor(NULL, &theme);
    test_color_equality(BLACK, null_token_result, "NULL token type handling");

    Color null_theme_result = GetSyntaxHighlightColor("key", NULL);
    test_color_equality(BLACK, null_theme_result, "NULL theme handling");
}

void test_color_blending() {
    printf("\n=== Testing Color Blending ===\n");

    Color red = {255, 0, 0, 255};
    Color blue = {0, 0, 255, 255};

    Color result_0 = BlendColors(red, blue, 0.0f);
    test_color_equality(red, result_0, "Alpha 0.0 blending");

    Color result_1 = BlendColors(red, blue, 1.0f);
    test_color_equality(blue, result_1, "Alpha 1.0 blending");

    Color result_half = BlendColors(red, blue, 0.5f);
    Color expected_half = {127, 0, 127, 255};
    test_color_equality(expected_half, result_half, "Alpha 0.5 blending");

    Color result_negative = BlendColors(red, blue, -0.5f);
    test_color_equality(red, result_negative, "Negative alpha clamping");

    Color result_over_one = BlendColors(red, blue, 1.5f);
    test_color_equality(blue, result_over_one, "Alpha > 1.0 clamping");
}

void test_accessibility() {
    printf("\n=== Testing Accessibility (Contrast Ratios) ===\n");

    GruvboxTheme theme = InitGruvboxTheme();

    double bg_fg_contrast = calculate_contrast_ratio(theme.bg0, theme.fg0);
    printf("Background/Foreground contrast ratio: %.2f\n", bg_fg_contrast);

    assert(bg_fg_contrast >= 4.5);
    printf("PASS: Background/Foreground meets WCAG AA standards (%.2f >= 4.5)\n", bg_fg_contrast);

    double error_contrast = calculate_contrast_ratio(theme.bg0, theme.red);
    printf("Background/Error contrast ratio: %.2f\n", error_contrast);
    assert(error_contrast >= 3.0);
    printf("PASS: Background/Error meets minimum contrast (%.2f >= 3.0)\n", error_contrast);

    double success_contrast = calculate_contrast_ratio(theme.bg0, theme.green);
    printf("Background/Success contrast ratio: %.2f\n", success_contrast);
    assert(success_contrast >= 3.0);
    printf("PASS: Background/Success meets minimum contrast (%.2f >= 3.0)\n", success_contrast);

    double key_contrast = calculate_contrast_ratio(theme.bg0, theme.blue);
    printf("Background/JSON Key contrast ratio: %.2f\n", key_contrast);
    assert(key_contrast >= 3.0);
    printf("PASS: Background/JSON Key meets minimum contrast (%.2f >= 3.0)\n", key_contrast);
}

int main() {
    printf("Running Theme System Tests...\n");

    test_theme_initialization();
    test_status_colors();
    test_syntax_highlighting();
    test_color_blending();
    test_accessibility();

    printf("\n=== All Theme Tests Passed! ===\n");
    return 0;
}