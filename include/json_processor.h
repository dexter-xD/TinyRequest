#ifndef JSON_PROCESSOR_H
#define JSON_PROCESSOR_H

#include "raylib.h"
#include "theme.h"
#include <cjson.h>
#include <stdio.h>

typedef struct MemoryPool MemoryPool;

typedef enum {
    JSON_TOKEN_KEY,
    JSON_TOKEN_STRING,
    JSON_TOKEN_NUMBER,
    JSON_TOKEN_BOOLEAN,
    JSON_TOKEN_NULL,
    JSON_TOKEN_BRACE,
    JSON_TOKEN_BRACKET,
    JSON_TOKEN_COMMA,
    JSON_TOKEN_COLON,
    JSON_TOKEN_WHITESPACE
} JsonTokenType;

typedef struct {
    JsonTokenType type;
    char* text;
    int start_pos;
    int length;
    Color color;
} JsonToken;

int ValidateJson(const char* json_string);
char* FormatJson(const char* json_string);
char* MinifyJson(const char* json_string);

JsonToken* TokenizeJson(const char* json_string, int* token_count);
JsonToken* TokenizeJsonOptimized(const char* json_string, int* token_count, MemoryPool* pool);
void FreeJsonTokens(JsonToken* tokens, int token_count);
void RenderJsonWithHighlighting(Rectangle bounds, const char* json, GruvboxTheme* theme);
void RenderJsonLine(Vector2 position, const char* line, GruvboxTheme* theme);
void RenderTokensChunked(Rectangle bounds, JsonToken* tokens, int token_count, GruvboxTheme* theme);
void RenderLargeJsonOptimized(Rectangle bounds, const char* json, GruvboxTheme* theme);

cJSON* ParseJsonSafely(const char* json_string);
char* GetJsonErrorMessage(void);
int GetJsonErrorPosition(void);

void DrawJsonText(Vector2 position, const char* json, Font font, GruvboxTheme* theme);
void DrawJsonTextWithHighlighting(Vector2 position, const char* json, GruvboxTheme* theme);
Vector2 MeasureJsonText(const char* json, Font font);

#endif