#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "../include/http_client.h"

void test_timeout_handling() {
    printf("Testing timeout handling...\n");

    assert(InitHttpClient() == 1);

    HttpRequest* request = CreateHttpRequest();
    assert(request != NULL);

    SetHttpRequestUrl(request, "https://httpbin.org/delay/35");
    SetHttpRequestMethod(request, "GET");

    AsyncHttpHandle* handle = SendHttpRequestAsync(request);
    assert(handle != NULL);

    handle->state = ASYNC_REQUEST_PENDING;
    handle->start_time = ((double)clock() / CLOCKS_PER_SEC) - 31.0;

    AsyncRequestState state = CheckAsyncRequest(handle);
    assert(state == ASYNC_REQUEST_TIMEOUT);
    assert(strlen(handle->error_message) > 0);
    assert(strstr(handle->error_message, "timeout") != NULL);

    printf("✓ Timeout handling works correctly\n");
    printf("  Error message: %s\n", handle->error_message);

    HttpError error = GetLastHttpError();
    assert(error.code == HTTP_ERROR_TIMEOUT);
    printf("✓ Timeout error code set correctly: %s\n", GetHttpErrorString(error.code));

    FreeAsyncHandle(handle);
    FreeHttpRequest(request);
    CleanupHttpClient();

    printf("✓ Timeout handling test passed\n\n");
}

void test_cancellation() {
    printf("Testing request cancellation...\n");

    assert(InitHttpClient() == 1);

    HttpRequest* request = CreateHttpRequest();
    assert(request != NULL);

    SetHttpRequestUrl(request, "https://httpbin.org/get");
    SetHttpRequestMethod(request, "GET");

    AsyncHttpHandle* handle = SendHttpRequestAsync(request);
    assert(handle != NULL);

    handle->state = ASYNC_REQUEST_PENDING;

    CancelAsyncRequest(handle);
    assert(handle->cancelled == 1);

    AsyncRequestState state = CheckAsyncRequest(handle);
    assert(state == ASYNC_REQUEST_CANCELLED);
    assert(strlen(handle->error_message) > 0);
    assert(strstr(handle->error_message, "cancelled") != NULL);

    printf("✓ Request cancellation works correctly\n");
    printf("  Error message: %s\n", handle->error_message);

    handle->state = ASYNC_REQUEST_COMPLETED;
    handle->cancelled = 0;
    CancelAsyncRequest(handle);
    assert(handle->cancelled == 1);
    state = CheckAsyncRequest(handle);
    assert(state == ASYNC_REQUEST_COMPLETED);

    printf("✓ Cancellation of completed request handled correctly\n");

    FreeAsyncHandle(handle);
    FreeHttpRequest(request);
    CleanupHttpClient();

    printf("✓ Cancellation test passed\n\n");
}

void test_progress_tracking() {
    printf("Testing progress tracking...\n");

    assert(InitHttpClient() == 1);

    HttpRequest* request = CreateHttpRequest();
    assert(request != NULL);

    SetHttpRequestUrl(request, "https://httpbin.org/get");
    SetHttpRequestMethod(request, "GET");

    AsyncHttpHandle* handle = SendHttpRequestAsync(request);
    assert(handle != NULL);

    assert(handle->start_time > 0);
    printf("✓ Start time recorded: %.3f\n", handle->start_time);

    printf("✓ Initial state: %d\n", handle->state);

    for (int i = 0; i < 3; i++) {
        AsyncRequestState state = CheckAsyncRequest(handle);
        printf("✓ Check %d - State: %d\n", i + 1, state);

        clock_t start = clock();
        while (((double)(clock() - start)) / CLOCKS_PER_SEC < 0.001) {

        }
    }

    if (handle->state == ASYNC_REQUEST_ERROR) {
        assert(strlen(handle->error_message) > 0);
        printf("✓ Error message set: %s\n", handle->error_message);
    }

    FreeAsyncHandle(handle);
    FreeHttpRequest(request);
    CleanupHttpClient();

    printf("✓ Progress tracking test passed\n\n");
}

void test_error_states() {
    printf("Testing error state management...\n");

    assert(InitHttpClient() == 1);

    const char* error_strings[] = {
        "No error",
        "Invalid URL format",
        "Network connection failed",
        "Request timeout",
        "SSL/TLS error",
        "Out of memory",
        "Request cancelled",
        "Unknown error"
    };

    for (int i = 0; i < 8; i++) {
        const char* error_str = GetHttpErrorString((HttpErrorCode)i);
        assert(strcmp(error_str, error_strings[i]) == 0);
        printf("✓ Error code %d: %s\n", i, error_str);
    }

    SetHttpError(HTTP_ERROR_NETWORK_FAILURE, "Test error");
    HttpError error = GetLastHttpError();
    assert(error.code == HTTP_ERROR_NETWORK_FAILURE);
    assert(strcmp(error.message, "Test error") == 0);

    ClearHttpError();
    error = GetLastHttpError();
    assert(error.code == HTTP_ERROR_NONE);
    assert(strlen(error.message) == 0);

    printf("✓ Error clearing works correctly\n");

    CleanupHttpClient();

    printf("✓ Error state management test passed\n\n");
}

void test_async_handle_lifecycle() {
    printf("Testing async handle lifecycle...\n");

    assert(InitHttpClient() == 1);

    AsyncHttpHandle* handle = SendHttpRequestAsync(NULL);
    assert(handle == NULL);
    HttpError error = GetLastHttpError();
    assert(error.code == HTTP_ERROR_INVALID_URL);
    printf("✓ NULL request handled correctly\n");

    HttpRequest* request = CreateHttpRequest();
    SetHttpRequestUrl(request, "invalid-url");
    handle = SendHttpRequestAsync(request);
    if (handle) {
        assert(handle->state == ASYNC_REQUEST_ERROR);
        FreeAsyncHandle(handle);
    }
    error = GetLastHttpError();
    assert(error.code == HTTP_ERROR_INVALID_URL);
    printf("✓ Invalid URL handled correctly\n");

    SetHttpRequestUrl(request, "https://httpbin.org/get");
    handle = SendHttpRequestAsync(request);
    assert(handle != NULL);

    assert(handle->request == request);
    assert(handle->start_time >= 0);
    assert(handle->cancelled == 0);

    FreeAsyncHandle(handle);
    printf("✓ Handle cleanup works correctly\n");

    FreeHttpRequest(request);
    CleanupHttpClient();

    printf("✓ Async handle lifecycle test passed\n\n");
}

int main() {
    printf("=== Progress and Loading Features Tests ===\n\n");

    test_error_states();
    test_async_handle_lifecycle();
    test_progress_tracking();
    test_timeout_handling();
    test_cancellation();

    printf("=== All progress feature tests passed! ===\n");
    return 0;
}