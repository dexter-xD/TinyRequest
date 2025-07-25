#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void print_state_info(RequestState* state) {
    printf("=== Request State ===\n");
    printf("Method: %s\n", GetHttpMethodString(state->selected_method));
    printf("URL: %s\n", state->url);
    printf("Body: %s\n", strlen(state->body) > 0 ? state->body : "(empty)");
    printf("Headers (%d):\n", state->header_count);
    for (int i = 0; i < state->header_count; i++) {
        printf("  %s\n", state->headers[i]);
    }
    printf("Status: %s\n", state->status_message);
    printf("Request in progress: %s\n", state->request_in_progress ? "Yes" : "No");
    if (state->last_request_time > 0) {
        printf("Last request time: %.3f seconds\n", state->last_request_time);
    }
    printf("====================\n\n");
}

int main() {
    printf("HTTP Request Tester - State Management Demo\n");
    printf("==========================================\n\n");

    printf("1. Creating new request state...\n");
    RequestState* state = CreateRequestState();
    if (!state) {
        printf("Failed to create request state!\n");
        return 1;
    }
    print_state_info(state);

    printf("2. Configuring POST request...\n");
    state->selected_method = HTTP_POST;
    strcpy(state->url, "https://jsonplaceholder.typicode.com/posts");
    strcpy(state->body, "{\"title\": \"Demo Post\", \"body\": \"This is a demo\", \"userId\": 1}");
    strcpy(state->headers[0], "Content-Type: application/json");
    strcpy(state->headers[1], "Accept: application/json");
    strcpy(state->headers[2], "User-Agent: TinyRequest/1.0");
    state->header_count = 3;
    print_state_info(state);

    printf("3. Validating request state...\n");
    if (IsValidRequestState(state)) {
        printf("✓ Request state is valid\n\n");
    } else {
        char* error = GetStateValidationError(state);
        printf("✗ Request state is invalid: %s\n\n", error);
        free(error);
    }

    printf("4. Building HTTP request from state...\n");
    HttpRequest* request = BuildHttpRequestFromState(state);
    if (request) {
        printf("✓ HTTP request built successfully\n");
        printf("  Method: %s\n", request->method);
        printf("  URL: %s\n", request->url);
        printf("  Body: %s\n", request->body);
        printf("  Headers: %d\n", request->header_count);

        free(request->url);
        free(request->method);
        free(request->body);
        for (int i = 0; i < request->header_count; i++) {
            free(request->headers[i]);
        }
        free(request->headers);
        free(request);
    } else {
        printf("✗ Failed to build HTTP request\n");
    }
    printf("\n");

    printf("5. Saving state to default location...\n");
    if (SaveDefaultState(state) == 0) {
        printf("✓ State saved to: %s\n\n", GetDefaultStateFilePath());
    } else {
        printf("✗ Failed to save state\n\n");
    }

    printf("6. Simulating HTTP response...\n");
    HttpResponse* response = malloc(sizeof(HttpResponse));
    response->status_code = 201;
    response->response_time = 0.234;
    response->body = strdup("{\"id\": 101, \"title\": \"Demo Post\", \"body\": \"This is a demo\", \"userId\": 1}");
    response->headers = malloc(2 * sizeof(char*));
    response->headers[0] = strdup("Content-Type: application/json");
    response->headers[1] = strdup("Server: nginx/1.18.0");
    response->header_count = 2;

    UpdateStateFromResponse(state, response);
    print_state_info(state);

    printf("7. Loading state from saved file...\n");
    RequestState* loaded_state = CreateRequestState();
    if (LoadDefaultState(loaded_state) == 0) {
        printf("✓ State loaded successfully\n");
        print_state_info(loaded_state);
    } else {
        printf("✗ Failed to load state\n");
    }

    printf("8. Clearing request history...\n");
    ClearRequestHistory(loaded_state);
    print_state_info(loaded_state);

    printf("9. Resetting state to defaults...\n");
    ResetRequestState(loaded_state);
    print_state_info(loaded_state);

    FreeRequestState(state);
    FreeRequestState(loaded_state);

    printf("Demo completed successfully!\n");
    return 0;
}