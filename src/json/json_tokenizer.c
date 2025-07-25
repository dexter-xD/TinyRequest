#include "json_processor.h"
#include "memory_manager.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

JsonToken* TokenizeJson(const char* json_string, int* token_count) {
    if (!json_string || !token_count) {
        if (token_count) *token_count = 0;
        return NULL;
    }

    int len = strlen(json_string);
    if (len == 0) {
        *token_count = 0;
        return NULL;
    }

    int capacity = 100;
    JsonToken* tokens = malloc(capacity * sizeof(JsonToken));
    if (!tokens) {
        *token_count = 0;
        return NULL;
    }

    int count = 0;
    int i = 0;

    while (i < len) {

        if (count >= capacity) {
            capacity *= 2;
            JsonToken* new_tokens = realloc(tokens, capacity * sizeof(JsonToken));
            if (!new_tokens) {
                FreeJsonTokens(tokens, count);
                *token_count = 0;
                return NULL;
            }
            tokens = new_tokens;
        }

        char c = json_string[i];
        JsonToken* token = &tokens[count];
        token->start_pos = i;

        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            int start = i;
            while (i < len && (json_string[i] == ' ' || json_string[i] == '\t' ||
                              json_string[i] == '\n' || json_string[i] == '\r')) {
                i++;
            }
            token->type = JSON_TOKEN_WHITESPACE;
            token->length = i - start;
            token->text = malloc(token->length + 1);
            if (token->text) {
                strncpy(token->text, &json_string[start], token->length);
                token->text[token->length] = '\0';
            }
            count++;
            continue;
        }

        if (c == '"') {
            int start = i;
            i++;
            bool escaped = false;

            while (i < len) {
                if (escaped) {
                    escaped = false;
                } else if (json_string[i] == '\\') {
                    escaped = true;
                } else if (json_string[i] == '"') {
                    i++;
                    break;
                }
                i++;
            }

            int j = i;
            while (j < len && (json_string[j] == ' ' || json_string[j] == '\t')) j++;
            token->type = (j < len && json_string[j] == ':') ? JSON_TOKEN_KEY : JSON_TOKEN_STRING;

            token->length = i - start;
            token->text = malloc(token->length + 1);
            if (token->text) {
                strncpy(token->text, &json_string[start], token->length);
                token->text[token->length] = '\0';
            }
            count++;
            continue;
        }

        if (isdigit(c) || c == '-' || c == '+') {
            int start = i;
            if (c == '-' || c == '+') i++;

            while (i < len && (isdigit(json_string[i]) || json_string[i] == '.' ||
                              json_string[i] == 'e' || json_string[i] == 'E' ||
                              json_string[i] == '+' || json_string[i] == '-')) {
                i++;
            }

            token->type = JSON_TOKEN_NUMBER;
            token->length = i - start;
            token->text = malloc(token->length + 1);
            if (token->text) {
                strncpy(token->text, &json_string[start], token->length);
                token->text[token->length] = '\0';
            }
            count++;
            continue;
        }

        if (strncmp(&json_string[i], "true", 4) == 0) {
            token->type = JSON_TOKEN_BOOLEAN;
            token->length = 4;
            token->text = malloc(5);
            if (token->text) strcpy(token->text, "true");
            i += 4;
            count++;
            continue;
        }

        if (strncmp(&json_string[i], "false", 5) == 0) {
            token->type = JSON_TOKEN_BOOLEAN;
            token->length = 5;
            token->text = malloc(6);
            if (token->text) strcpy(token->text, "false");
            i += 5;
            count++;
            continue;
        }

        if (strncmp(&json_string[i], "null", 4) == 0) {
            token->type = JSON_TOKEN_NULL;
            token->length = 4;
            token->text = malloc(5);
            if (token->text) strcpy(token->text, "null");
            i += 4;
            count++;
            continue;
        }

        switch (c) {
            case '{':
            case '}':
                token->type = JSON_TOKEN_BRACE;
                break;
            case '[':
            case ']':
                token->type = JSON_TOKEN_BRACKET;
                break;
            case ',':
                token->type = JSON_TOKEN_COMMA;
                break;
            case ':':
                token->type = JSON_TOKEN_COLON;
                break;
            default:

                i++;
                continue;
        }

        token->length = 1;
        token->text = malloc(2);
        if (token->text) {
            token->text[0] = c;
            token->text[1] = '\0';
        }
        i++;
        count++;
    }

    *token_count = count;
    return tokens;
}

JsonToken* TokenizeJsonOptimized(const char* json_string, int* token_count, MemoryPool* pool) {
    if (!json_string || !token_count || !pool) {
        if (token_count) *token_count = 0;
        return NULL;
    }

    int len = strlen(json_string);
    if (len == 0) {
        *token_count = 0;
        return NULL;
    }

    int estimated_tokens = len / 10;
    if (estimated_tokens < 100) estimated_tokens = 100;

    JsonToken* tokens = (JsonToken*)PoolAlloc(pool, estimated_tokens * sizeof(JsonToken));
    if (!tokens) {
        *token_count = 0;
        return NULL;
    }

    int count = 0;
    int capacity = estimated_tokens;
    int i = 0;

    while (i < len && count < capacity - 1) {
        char c = json_string[i];
        JsonToken* token = &tokens[count];
        token->start_pos = i;

        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            int start = i;
            while (i < len && (json_string[i] == ' ' || json_string[i] == '\t' ||
                              json_string[i] == '\n' || json_string[i] == '\r')) {
                i++;
            }
            token->type = JSON_TOKEN_WHITESPACE;
            token->length = i - start;
            token->text = (char*)PoolAlloc(pool, token->length + 1);
            if (token->text) {
                strncpy(token->text, &json_string[start], token->length);
                token->text[token->length] = '\0';
            }
            count++;
            continue;
        }

        if (c == '"') {
            int start = i;
            i++;
            bool escaped = false;

            while (i < len) {
                if (escaped) {
                    escaped = false;
                } else if (json_string[i] == '\\') {
                    escaped = true;
                } else if (json_string[i] == '"') {
                    i++;
                    break;
                }
                i++;
            }

            int j = i;
            while (j < len && (json_string[j] == ' ' || json_string[j] == '\t')) j++;
            token->type = (j < len && json_string[j] == ':') ? JSON_TOKEN_KEY : JSON_TOKEN_STRING;

            token->length = i - start;
            token->text = (char*)PoolAlloc(pool, token->length + 1);
            if (token->text) {
                strncpy(token->text, &json_string[start], token->length);
                token->text[token->length] = '\0';
            }
            count++;
            continue;
        }

        if (isdigit(c) || c == '-' || c == '+') {
            int start = i;
            if (c == '-' || c == '+') i++;

            while (i < len && (isdigit(json_string[i]) || json_string[i] == '.' ||
                              json_string[i] == 'e' || json_string[i] == 'E' ||
                              json_string[i] == '+' || json_string[i] == '-')) {
                i++;
            }

            token->type = JSON_TOKEN_NUMBER;
            token->length = i - start;
            token->text = (char*)PoolAlloc(pool, token->length + 1);
            if (token->text) {
                strncpy(token->text, &json_string[start], token->length);
                token->text[token->length] = '\0';
            }
            count++;
            continue;
        }

        if (strncmp(&json_string[i], "true", 4) == 0) {
            token->type = JSON_TOKEN_BOOLEAN;
            token->length = 4;
            token->text = (char*)PoolAlloc(pool, 5);
            if (token->text) strcpy(token->text, "true");
            i += 4;
            count++;
            continue;
        }

        if (strncmp(&json_string[i], "false", 5) == 0) {
            token->type = JSON_TOKEN_BOOLEAN;
            token->length = 5;
            token->text = (char*)PoolAlloc(pool, 6);
            if (token->text) strcpy(token->text, "false");
            i += 5;
            count++;
            continue;
        }

        if (strncmp(&json_string[i], "null", 4) == 0) {
            token->type = JSON_TOKEN_NULL;
            token->length = 4;
            token->text = (char*)PoolAlloc(pool, 5);
            if (token->text) strcpy(token->text, "null");
            i += 4;
            count++;
            continue;
        }

        switch (c) {
            case '{':
            case '}':
                token->type = JSON_TOKEN_BRACE;
                break;
            case '[':
            case ']':
                token->type = JSON_TOKEN_BRACKET;
                break;
            case ',':
                token->type = JSON_TOKEN_COMMA;
                break;
            case ':':
                token->type = JSON_TOKEN_COLON;
                break;
            default:

                i++;
                continue;
        }

        token->length = 1;
        token->text = (char*)PoolAlloc(pool, 2);
        if (token->text) {
            token->text[0] = c;
            token->text[1] = '\0';
        }
        i++;
        count++;
    }

    *token_count = count;
    return tokens;
}

void FreeJsonTokens(JsonToken* tokens, int token_count) {
    if (!tokens) return;

    for (int i = 0; i < token_count; i++) {
        if (tokens[i].text) {
            free(tokens[i].text);
        }
    }

    free(tokens);
}