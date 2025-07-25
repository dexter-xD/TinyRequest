#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/http_client.h"

void test_async_http_request() {
    printf("Testing async HTTP request functionality...\n");

    assert(InitHttpClient() == 1);

    HttpRequest* request = CreateHttpRequest();
    assert(request != NULL);

    SetHttpRequestUrl(request, "https://httpbin.org/get");
    SetHttpRequestMethod(request, "GET");

    AsyncHttpHandle* handle = SendHttpRequestAsync(request);
    assert(handle != NULL);

    AsyncRequestState state = CheckAsyncRequest(handle);
    assert(state == ASYNC_REQUEST_COMPLETED || state == ASYNC_REQUEST_ERROR);

    if (state == ASYNC_REQUEST_COMPLETED) {

        HttpResponse* response = GetAsyncResponse(handle);
        assert(response != NULL);
        assert(response->status_code == 200);
        assert(response->body != NULL);
        assert(strlen(response->body) > 0);

        printf("✓ Async request completed successfully\n");
        printf("  Status: %d\n", response->status_code);
        printf("  Response time: %.2f ms\n", response->response_time);
        printf("  Body length: %zu bytes\n", strlen(response->body));

        FreeHttpResponse(response);
    } else {
        printf("✓ Async request handled error state correctly\n");
        printf("  Error: %s\n", handle->error_message);
    }

    FreeAsyncHandle(handle);
    FreeHttpRequest(request);
    CleanupHttpClient();

    printf("✓ Async HTTP request test passed\n\n");
}

void test_error_handling() {
    printf("Testing error handling...\n");

    assert(InitHttpClient() == 1);

    HttpRequest* request = CreateHttpRequest();
    SetHttpRequestUrl(request, "invalid-url");

    AsyncHttpHandle* handle = SendHttpRequestAsync(request);
    if (handle) {
        assert(handle->state == ASYNC_REQUEST_ERROR);
        printf("✓ Invalid URL error handled correctly\n");
        FreeAsyncHandle(handle);
    }

    HttpError error = GetLastHttpError();
    assert(error.code == HTTP_ERROR_INVALID_URL);
    printf("✓ Error code set correctly: %s\n", GetHttpErrorString(error.code));

    ClearHttpError();
    error = GetLastHttpError();
    assert(error.code == HTTP_ERROR_NONE);
    printf("✓ Error cleared successfully\n");

    FreeHttpRequest(request);
    CleanupHttpClient();

    printf("✓ Error handling test passed\n\n");
}

void test_request_builder() {
    printf("Testing request builder from state...\n");

    HttpRequest* request = CreateHttpRequest();
    assert(request != NULL);

    SetHttpRequestUrl(request, "https://httpbin.org/post");
    SetHttpRequestMethod(request, "POST");
    SetHttpRequestBody(request, "{\"test\": \"data\"}");
    AddHttpRequestHeader(request, "Content-Type", "application/json");
    AddHttpRequestHeader(request, "Authorization", "Bearer token123");

    assert(strcmp(request->url, "https://httpbin.org/post") == 0);
    assert(strcmp(request->method, "POST") == 0);
    assert(strcmp(request->body, "{\"test\": \"data\"}") == 0);
    assert(request->header_count == 2);
    assert(strstr(request->headers[0], "Content-Type: application/json") != NULL);
    assert(strstr(request->headers[1], "Authorization: Bearer token123") != NULL);

    printf("✓ Request built correctly with all components\n");
    printf("  URL: %s\n", request->url);
    printf("  Method: %s\n", request->method);
    printf("  Body: %s\n", request->body);
    printf("  Headers: %d\n", request->header_count);

    FreeHttpRequest(request);

    printf("✓ Request builder test passed\n\n");
}

int main() {
    printf("=== Async HTTP Client Tests ===\n\n");

    test_request_builder();
    test_error_handling();
    test_async_http_request();

    printf("=== All tests passed! ===\n");
    return 0;
}