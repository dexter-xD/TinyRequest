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

void test_default_state_path() {
    printf("Testing GetDefaultStateFilePath...\n");

    const char* path = GetDefaultStateFilePath();
    assert(path != NULL);
    assert(strlen(path) > 0);

    assert(strstr(path, "tinyrequest_state") != NULL);

    printf("Default state path: %s\n", path);
    printf("✓ GetDefaultStateFilePath test passed\n");
}

void test_default_state_save_load() {
    printf("Testing SaveDefaultState and LoadDefaultState...\n");

    RequestState* state1 = CreateRequestState();
    RequestState* state2 = CreateRequestState();

    state1->selected_method = HTTP_PUT;
    strcpy(state1->url, "https://api.test.com/endpoint");
    strcpy(state1->body, "{\"test\": \"default_state\"}");
    strcpy(state1->headers[0], "X-Test-Header: test-value");
    state1->header_count = 1;

    int save_result = SaveDefaultState(state1);
    assert(save_result == 0);

    int load_result = LoadDefaultState(state2);
    assert(load_result == 0);

    assert(state2->selected_method == HTTP_PUT);
    assert(strcmp(state2->url, "https://api.test.com/endpoint") == 0);
    assert(strcmp(state2->body, "{\"test\": \"default_state\"}") == 0);
    assert(state2->header_count == 1);
    assert(strcmp(state2->headers[0], "X-Test-Header: test-value") == 0);

    FreeRequestState(state1);
    FreeRequestState(state2);

    printf("✓ SaveDefaultState and LoadDefaultState test passed\n");
}

void test_clear_request_history() {
    printf("Testing ClearRequestHistory...\n");

    RequestState* state = CreateRequestState();

    HttpResponse* response = malloc(sizeof(HttpResponse));
    response->status_code = 200;
    response->response_time = 0.456;
    response->body = strdup("Test response");
    response->headers = NULL;
    response->header_count = 0;

    state->last_response = response;
    state->last_request_time = 0.456;
    strcpy(state->status_message, "Previous request completed");

    ClearRequestHistory(state);

    assert(state->last_response == NULL);
    assert(state->last_request_time == 0.0);
    assert(strcmp(state->status_message, "Ready") == 0);

    FreeRequestState(state);
    printf("✓ ClearRequestHistory test passed\n");
}

void test_comprehensive_state_management() {
    printf("Testing comprehensive state management workflow...\n");

    RequestState* state = CreateRequestState();

    state->selected_method = HTTP_POST;
    strcpy(state->url, "https://jsonplaceholder.typicode.com/posts");
    strcpy(state->body, "{\"title\": \"Test Post\", \"body\": \"Test content\", \"userId\": 1}");
    strcpy(state->headers[0], "Content-Type: application/json");
    strcpy(state->headers[1], "Accept: application/json");
    state->header_count = 2;

    assert(IsValidRequestState(state) == true);
    char* error = GetStateValidationError(state);
    assert(error == NULL);

    HttpRequest* request = BuildHttpRequestFromState(state);
    assert(request != NULL);
    assert(strcmp(request->method, "POST") == 0);
    assert(strcmp(request->url, "https://jsonplaceholder.typicode.com/posts") == 0);

    int save_result = SaveDefaultState(state);
    assert(save_result == 0);

    RequestState* loaded_state = CreateRequestState();
    int load_result = LoadDefaultState(loaded_state);
    assert(load_result == 0);

    assert(loaded_state->selected_method == HTTP_POST);
    assert(strcmp(loaded_state->url, "https://jsonplaceholder.typicode.com/posts") == 0);
    assert(strcmp(loaded_state->body, "{\"title\": \"Test Post\", \"body\": \"Test content\", \"userId\": 1}") == 0);
    assert(loaded_state->header_count == 2);

    HttpResponse* response = malloc(sizeof(HttpResponse));
    response->status_code = 201;
    response->response_time = 0.234;
    response->body = strdup("{\"id\": 101, \"title\": \"Test Post\"}");
    response->headers = NULL;
    response->header_count = 0;

    UpdateStateFromResponse(loaded_state, response);
    assert(loaded_state->last_response == response);
    assert(loaded_state->request_in_progress == false);
    assert(strstr(loaded_state->status_message, "Success: 201") != NULL);

    ClearRequestHistory(loaded_state);
    assert(loaded_state->last_response == NULL);
    assert(strcmp(loaded_state->status_message, "Ready") == 0);

    free(request->url);
    free(request->method);
    free(request->body);
    for (int i = 0; i < request->header_count; i++) {
        free(request->headers[i]);
    }
    free(request->headers);
    free(request);
    FreeRequestState(state);
    FreeRequestState(loaded_state);

    printf("✓ Comprehensive state management workflow test passed\n");
}

int main() {
    printf("Running state utility tests...\n\n");

    test_default_state_path();
    test_default_state_save_load();
    test_clear_request_history();
    test_comprehensive_state_management();

    printf("\n✓ All state utility tests passed!\n");
    return 0;
}