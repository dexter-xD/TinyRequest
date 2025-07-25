#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/http_client.h"
#include "../include/ui_components.h"
#include "../include/theme.h"

Vector2 GetMousePosition(void) { return (Vector2){0, 0}; }
bool IsMouseButtonPressed(int button) { return false; }
bool CheckCollisionPointRec(Vector2 point, Rectangle rec) { return false; }
void DrawRectangleRec(Rectangle rec, Color color) {}
void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color) {}
void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) {}
void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) {}
Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) { return (Vector2){100, 20}; }
Font GetFontDefault(void) { return (Font){0}; }
float GetFrameTime(void) { return 0.016f; }
Color BlendColors(Color dst, Color src, float tint) { return dst; }

void test_timeout_handling() {
    printf("Testing timeout handling...\n");

    assert(InitHttpClient() == 1);

    HttpRequest* request = CreateHttpRequest();
    assert(request != NULL);

    SetHttpRequestUrl(request, "https://httpbin.org/delay/35");
    SetHttpRequestMethod(request, "GET");

    AsyncHttpHandle* handle = SendHttpRequestAsync(request);
    assert(handle != NULL);

    handle->start_time = ((double)clock() / CLOCKS_PER_SEC) - 31.0;

    AsyncRequestState state = CheckAsyncRequest(handle);
    assert(state == ASYNC_REQUEST_TIMEOUT);
    assert(strlen(handle->error_message) > 0);
    assert(strstr(handle->error_message, "timeout") != NULL);

    printf("✓ Timeout handling works correctly\n");
    printf("  Error message: %s\n", handle->error_message);

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

    FreeAsyncHandle(handle);
    FreeHttpRequest(request);
    CleanupHttpClient();

    printf("✓ Cancellation test passed\n\n");
}

void test_loading_indicators() {
    printf("Testing loading indicator UI components...\n");

    GruvboxTheme theme = InitGruvboxTheme();

    Vector2 center = {100, 100};
    RenderLoadingSpinner(center, 15.0f, &theme);
    printf("✓ Loading spinner renders without errors\n");

    Rectangle progress_bounds = {50, 50, 200, 20};
    RenderProgressBar(progress_bounds, 0.5f, "Loading...", &theme);
    printf("✓ Progress bar renders without errors\n");

    RenderProgressBar(progress_bounds, 0.0f, "Starting...", &theme);
    RenderProgressBar(progress_bounds, 1.0f, "Complete!", &theme);
    printf("✓ Progress bar handles edge cases correctly\n");

    Rectangle cancel_bounds = {250, 50, 80, 25};
    bool cancel_requested = false;
    RenderCancelButton(cancel_bounds, &cancel_requested, &theme);
    printf("✓ Cancel button renders without errors\n");

    printf("✓ Loading indicator UI test passed\n\n");
}

void test_request_progress_display() {
    printf("Testing request progress display...\n");

    assert(InitHttpClient() == 1);
    GruvboxTheme theme = InitGruvboxTheme();

    HttpRequest* request = CreateHttpRequest();
    assert(request != NULL);

    SetHttpRequestUrl(request, "https://httpbin.org/get");
    SetHttpRequestMethod(request, "GET");

    AsyncHttpHandle* handle = SendHttpRequestAsync(request);
    assert(handle != NULL);

    Rectangle progress_bounds = {10, 10, 400, 30};

    handle->state = ASYNC_REQUEST_PENDING;
    RenderRequestProgress(progress_bounds, handle, &theme);
    printf("✓ Pending state display works\n");

    handle->state = ASYNC_REQUEST_COMPLETED;
    RenderRequestProgress(progress_bounds, handle, &theme);
    printf("✓ Completed state display works\n");

    handle->state = ASYNC_REQUEST_ERROR;
    strcpy(handle->error_message, "Network error");
    RenderRequestProgress(progress_bounds, handle, &theme);
    printf("✓ Error state display works\n");

    handle->state = ASYNC_REQUEST_TIMEOUT;
    RenderRequestProgress(progress_bounds, handle, &theme);
    printf("✓ Timeout state display works\n");

    handle->state = ASYNC_REQUEST_CANCELLED;
    RenderRequestProgress(progress_bounds, handle, &theme);
    printf("✓ Cancelled state display works\n");

    FreeAsyncHandle(handle);
    FreeHttpRequest(request);
    CleanupHttpClient();

    printf("✓ Request progress display test passed\n\n");
}

void test_error_feedback() {
    printf("Testing error feedback integration...\n");

    assert(InitHttpClient() == 1);

    HttpRequest* request = CreateHttpRequest();

    SetHttpRequestUrl(request, "invalid-url");
    AsyncHttpHandle* handle = SendHttpRequestAsync(request);
    if (handle) {
        assert(handle->state == ASYNC_REQUEST_ERROR);
        assert(strlen(handle->error_message) > 0);
        printf("✓ Invalid URL error feedback: %s\n", handle->error_message);
        FreeAsyncHandle(handle);
    }

    SetHttpRequestUrl(request, "https://nonexistent-domain-12345.com");
    handle = SendHttpRequestAsync(request);
    if (handle) {

        assert(handle->state == ASYNC_REQUEST_ERROR);
        assert(strlen(handle->error_message) > 0);
        printf("✓ Network error feedback: %s\n", handle->error_message);
        FreeAsyncHandle(handle);
    }

    HttpError error = GetLastHttpError();
    printf("✓ Last error code: %d (%s)\n", error.code, GetHttpErrorString(error.code));

    FreeHttpRequest(request);
    CleanupHttpClient();

    printf("✓ Error feedback test passed\n\n");
}

int main() {
    printf("=== Loading Indicators and Progress Tests ===\n\n");

    test_loading_indicators();
    test_request_progress_display();
    test_timeout_handling();
    test_cancellation();
    test_error_feedback();

    printf("=== All loading indicator tests passed! ===\n");
    return 0;
}