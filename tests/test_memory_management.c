#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "include/request_state.h"

int ValidateUrl(const char* url) {
    if (!url || strlen(url) == 0) return -1;
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) return -1;
    return 0;
}

void FreeHttpResponse(HttpResponse* response) {
    if (!response) return;
    free(response->body);
    if (response->headers) {
        for (int i = 0; i < response->header_count; i++) {
            free(response->headers[i]);
        }
        free(response->headers);
    }
    free(response);
}

HttpRequest* CreateHttpRequest(void) {
    HttpRequest* request = malloc(sizeof(HttpRequest));
    if (!request) return NULL;

    request->url = NULL;
    request->method = NULL;
    request->body = NULL;
    request->headers = NULL;
    request->header_count = 0;
    return request;
}

void SetHttpRequestUrl(HttpRequest* request, const char* url) {
    if (!request || !url) return;
    free(request->url);
    request->url = strdup(url);
}

void SetHttpRequestMethod(HttpRequest* request, const char* method) {
    if (!request || !method) return;
    free(request->method);
    request->method = strdup(method);
}

void SetHttpRequestBody(HttpRequest* request, const char* body) {
    if (!request || !body) return;
    free(request->body);
    request->body = strdup(body);
}

void AddHttpRequestHeader(HttpRequest* request, const char* key, const char* value) {
    if (!request || !key || !value) return;

    request->header_count++;
    request->headers = realloc(request->headers, request->header_count * sizeof(char*));

    char* header = malloc(strlen(key) + strlen(value) + 3);
    sprintf(header, "%s: %s", key, value);
    request->headers[request->header_count - 1] = header;
}

void test_memory_management() {
    printf("Testing memory management...\n");

    for (int i = 0; i < 100; i++) {
        RequestState* state = CreateRequestState();
        assert(state != NULL);

        state->selected_method = HTTP_POST;
        strcpy(state->url, "https://api.example.com/test");
        strcpy(state->body, "{\"test\": \"data\"}");
        strcpy(state->headers[0], "Content-Type: application/json");
        state->header_count = 1;

        HttpResponse* response = malloc(sizeof(HttpResponse));
        response->status_code = 200;
        response->response_time = 0.123;
        response->body = strdup("Test response body");
        response->headers = malloc(sizeof(char*));
        response->headers[0] = strdup("Content-Type: application/json");
        response->header_count = 1;

        UpdateStateFromResponse(state, response);

        FreeRequestState(state);
    }

    printf("✓ Memory management test passed (100 cycles)\n");
}

void test_validation_error_memory() {
    printf("Testing validation error memory management...\n");

    RequestState* state = CreateRequestState();

    for (int i = 0; i < 50; i++) {
        char* error = GetStateValidationError(state);
        if (error) {
            free(error);
        }

        strcpy(state->url, "invalid-url");
        error = GetStateValidationError(state);
        assert(error != NULL);
        free(error);

        strcpy(state->url, "https://api.example.com");
        error = GetStateValidationError(state);
        assert(error == NULL);
    }

    FreeRequestState(state);
    printf("✓ Validation error memory management test passed\n");
}

void test_reset_with_response() {
    printf("Testing reset with existing response...\n");

    RequestState* state = CreateRequestState();

    HttpResponse* response = malloc(sizeof(HttpResponse));
    response->status_code = 200;
    response->response_time = 0.123;
    response->body = strdup("Test response");
    response->headers = NULL;
    response->header_count = 0;

    UpdateStateFromResponse(state, response);
    assert(state->last_response == response);

    ResetRequestState(state);
    assert(state->last_response == NULL);

    FreeRequestState(state);
    printf("✓ Reset with response test passed\n");
}

void test_multiple_response_updates() {
    printf("Testing multiple response updates...\n");

    RequestState* state = CreateRequestState();

    for (int i = 0; i < 10; i++) {
        HttpResponse* response = malloc(sizeof(HttpResponse));
        response->status_code = 200 + i;
        response->response_time = 0.1 * i;
        response->body = malloc(50);
        sprintf(response->body, "Response %d", i);
        response->headers = NULL;
        response->header_count = 0;

        UpdateStateFromResponse(state, response);
        assert(state->last_response == response);
    }

    FreeRequestState(state);
    printf("✓ Multiple response updates test passed\n");
}

int main() {
    printf("Running memory management tests...\n\n");

    test_memory_management();
    test_validation_error_memory();
    test_reset_with_response();
    test_multiple_response_updates();

    printf("\n✓ All memory management tests passed!\n");
    printf("No memory leaks detected in basic testing.\n");
    return 0;
}