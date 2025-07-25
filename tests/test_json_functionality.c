#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/json_processor.h"
#include "include/theme.h"

int main() {
    printf("Testing JSON functionality...\n");

    const char* valid_json = "{\"name\": \"test\", \"value\": 123, \"active\": true}";
    const char* invalid_json = "{\"name\": \"test\", \"value\": 123, \"active\": true";

    printf("Testing JSON validation:\n");
    printf("Valid JSON: %s\n", ValidateJson(valid_json) ? "PASS" : "FAIL");
    printf("Invalid JSON: %s\n", !ValidateJson(invalid_json) ? "PASS" : "FAIL");

    printf("\nTesting JSON formatting:\n");
    char* formatted = FormatJson(valid_json);
    if (formatted) {
        printf("Formatted JSON:\n%s\n", formatted);
        free(formatted);
        printf("JSON formatting: PASS\n");
    } else {
        printf("JSON formatting: FAIL\n");
    }

    printf("\nTesting JSON minification:\n");
    const char* formatted_json = "{\n  \"name\": \"test\",\n  \"value\": 123\n}";
    char* minified = MinifyJson(formatted_json);
    if (minified) {
        printf("Minified JSON: %s\n", minified);
        free(minified);
        printf("JSON minification: PASS\n");
    } else {
        printf("JSON minification: FAIL\n");
    }

    printf("\nTesting JSON tokenization:\n");
    int token_count = 0;
    JsonToken* tokens = TokenizeJson(valid_json, &token_count);
    if (tokens && token_count > 0) {
        printf("Tokenized %d tokens:\n", token_count);
        for (int i = 0; i < token_count && i < 10; i++) {
            const char* type_name = "UNKNOWN";
            switch (tokens[i].type) {
                case JSON_TOKEN_KEY: type_name = "KEY"; break;
                case JSON_TOKEN_STRING: type_name = "STRING"; break;
                case JSON_TOKEN_NUMBER: type_name = "NUMBER"; break;
                case JSON_TOKEN_BOOLEAN: type_name = "BOOLEAN"; break;
                case JSON_TOKEN_NULL: type_name = "NULL"; break;
                case JSON_TOKEN_BRACE: type_name = "BRACE"; break;
                case JSON_TOKEN_BRACKET: type_name = "BRACKET"; break;
                case JSON_TOKEN_COMMA: type_name = "COMMA"; break;
                case JSON_TOKEN_COLON: type_name = "COLON"; break;
                case JSON_TOKEN_WHITESPACE: type_name = "WHITESPACE"; break;
            }
            printf("  Token %d: %s = '%s'\n", i, type_name, tokens[i].text ? tokens[i].text : "NULL");
        }
        FreeJsonTokens(tokens, token_count);
        printf("JSON tokenization: PASS\n");
    } else {
        printf("JSON tokenization: FAIL\n");
    }

    printf("\nAll JSON functionality tests completed.\n");
    return 0;
}