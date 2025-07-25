#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/request_state.h"

int ValidateUrl(const char* url) {
    if (!url || strlen(url) == 0) return -1;
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) return -1;
    return 0;
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

void test_create_request_state() {
    printf("Testing CreateRequestState...\n");

    RequestState* state = CreateRequestState();
    assert(state != NULL);
    assert(state->selected_method == HTTP_GET);
    assert(strlen(state->url) == 0);
    assert(strlen(state->body) == 0);
    assert(state->header_count == 0);
    assert(state->last_response == NULL);
    assert(state->request_in_progress == false);
    assert(state->last_request_time == 0.0);
    assert(strcmp(state->status_message, "Ready") == 0);

    FreeRequestState(state);
    printf("✓ CreateRequestState test passed\n");
}

void test_reset_request_state() {
    printf("Testing ResetRequestState...\n");

    RequestState* state = CreateRequestState();

    state->selected_method = HTTP_POST;
    strcpy(state->url, "https://api.example.com");
    strcpy(state->body, "{\"test\": \"data\"}");
    strcpy(state->headers[0], "Content-Type: application/json");
    state->header_count = 1;
    state->request_in_progress = true;
    state->last_request_time = 123.45;
    strcpy(state->status_message, "Testing");

    ResetRequestState(state);

    assert(state->selected_method == HTTP_GET);
    assert(strlen(state->url) == 0);
    assert(strlen(state->body) == 0);
    assert(state->header_count == 0);
    assert(state->request_in_progress == false);
    assert(state->last_request_time == 0.0);
    assert(strcmp(state->status_message, "Ready") == 0);

    FreeRequestState(state);
    printf("✓ ResetRequestState test passed\n");
}

void test_save_load_request_state() {
    printf("Testing SaveRequestState and LoadRequestState...\n");

    RequestState* state1 = CreateRequestState();
    RequestState* state2 = CreateRequestState();

    state1->selected_method = HTTP_POST;
    strcpy(state1->url, "https://api.example.com/test");
    strcpy(state1->body, "{\"name\": \"test\", \"value\": 123}");
    strcpy(state1->headers[0], "Content-Type: application/json");
    strcpy(state1->headers[1], "Authorization: Bearer token123");
    state1->header_count = 2;

    const char* filename = "test_state.txt";
    int save_result = SaveRequestState(state1, filename);
    assert(save_result == 0);

    int load_result = LoadRequestState(state2, filename);
    assert(load_result == 0);

    assert(state2->selected_method == HTTP_POST);
    assert(strcmp(state2->url, "https://api.example.com/test") == 0);
    assert(strcmp(state2->body, "{\"name\": \"test\", \"value\": 123}") == 0);
    assert(state2->header_count == 2);
    assert(strcmp(state2->headers[0], "Content-Type: application/json") == 0);
    assert(strcmp(state2->headers[1], "Authorization: Bearer token123") == 0);

    remove(filename);
    FreeRequestState(state1);
    FreeRequestState(state2);
    printf("✓ SaveRequestState and LoadRequestState test passed\n");
}

void test_http_method_conversion() {
    printf("Testing HTTP method conversion functions...\n");

    assert(strcmp(GetHttpMethodString(HTTP_GET), "GET") == 0);
    assert(strcmp(GetHttpMethodString(HTTP_POST), "POST") == 0);
    assert(strcmp(GetHttpMethodString(HTTP_PUT), "PUT") == 0);
    assert(strcmp(GetHttpMethodString(HTTP_DELETE), "DELETE") == 0);

    assert(GetHttpMethodFromString("GET") == HTTP_GET);
    assert(GetHttpMethodFromString("POST") == HTTP_POST);
    assert(GetHttpMethodFromString("PUT") == HTTP_PUT);
    assert(GetHttpMethodFromString("DELETE") == HTTP_DELETE);
    assert(GetHttpMethodFromString("INVALID") == HTTP_GET);
    assert(GetHttpMethodFromString(NULL) == HTTP_GET);

    printf("✓ HTTP method conversion test passed\n");
}

void test_state_validation() {
    printf("Testing state validation functions...\n");

    RequestState* state = CreateRequestState();

    assert(IsValidRequestState(state) == false);
    char* error = GetStateValidationError(state);
    assert(error != NULL);
    assert(strcmp(error, "URL is required") == 0);
    free(error);

    strcpy(state->url, "https://api.example.com");
    assert(IsValidRequestState(state) == true);
    error = GetStateValidationError(state);
    assert(error == NULL);

    strcpy(state->url, "invalid-url");
    assert(IsValidRequestState(state) == false);
    error = GetStateValidationError(state);
    assert(error != NULL);
    assert(strcmp(error, "Invalid URL format") == 0);
    free(error);

    strcpy(state->url, "https://api.example.com");
    strcpy(state->headers[0], "InvalidHeaderWithoutColon");
    state->header_count = 1;
    assert(IsValidRequestState(state) == false);
    error = GetStateValidationError(state);
    assert(error != NULL);
    assert(strstr(error, "Invalid header format") != NULL);
    free(error);

    FreeRequestState(state);
    printf("✓ State validation test passed\n");
}

void test_build_http_request_from_state() {
    printf("Testing BuildHttpRequestFromState...\n");

    RequestState* state = CreateRequestState();

    state->selected_method = HTTP_POST;
    strcpy(state->url, "https://api.example.com/test");
    strcpy(state->body, "{\"test\": \"data\"}");
    strcpy(state->headers[0], "Content-Type: application/json");
    strcpy(state->headers[1], "Authorization: Bearer token");
    state->header_count = 2;

    HttpRequest* request = BuildHttpRequestFromState(state);
    assert(request != NULL);
    assert(strcmp(request->url, "https://api.example.com/test") == 0);
    assert(strcmp(request->method, "POST") == 0);
    assert(strcmp(request->body, "{\"test\": \"data\"}") == 0);
    assert(request->header_count == 2);

    free(request->url);
    free(request->method);
    free(request->body);
    for (int i = 0; i < request->header_count; i++) {
        free(request->headers[i]);
    }
    free(request->headers);
    free(request);
    FreeRequestState(state);
    printf("✓ BuildHttpRequestFromState test passed\n");
}

void test_status_message() {
    printf("Testing status message functions...\n");

    RequestState* state = CreateRequestState();

    SetStatusMessage(state, "Test message");
    assert(strcmp(state->status_message, "Test message") == 0);

    HttpResponse* response = malloc(sizeof(HttpResponse));
    response->status_code = 200;
    response->response_time = 0.123;
    response->body = strdup("Success");
    response->headers = NULL;
    response->header_count = 0;

    UpdateStateFromResponse(state, response);
    assert(state->last_response == response);
    assert(state->request_in_progress == false);
    assert(state->last_request_time == 0.123);
    assert(strstr(state->status_message, "Success: 200") != NULL);

    FreeRequestState(state);
    printf("✓ Status message test passed\n");
}

int main() {
    printf("Running request state management tests...\n\n");

    test_create_request_state();
    test_reset_request_state();
    test_save_load_request_state();
    test_http_method_conversion();
    test_state_validation();
    test_build_http_request_from_state();
    test_status_message();

    printf("\n✓ All request state management tests passed!\n");
    return 0;
}