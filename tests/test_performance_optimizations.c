#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "../include/memory_manager.h"
#include "../include/http_client.h"
#include "../include/json_processor.h"
#include "../include/ui_components.h"

void test_memory_pool_performance() {
    printf("Testing memory pool performance...\n");

    clock_t start = clock();

    MemoryPool* pool = CreateMemoryPool(64 * 1024);
    assert(pool != NULL);

    void* ptrs[1000];
    for (int i = 0; i < 1000; i++) {
        ptrs[i] = PoolAlloc(pool, 64);
        assert(ptrs[i] != NULL);
    }

    clock_t end = clock();
    double pool_time = ((double)(end - start)) / CLOCKS_PER_SEC;

    start = clock();
    void* malloc_ptrs[1000];
    for (int i = 0; i < 1000; i++) {
        malloc_ptrs[i] = malloc(64);
        assert(malloc_ptrs[i] != NULL);
    }
    end = clock();
    double malloc_time = ((double)(end - start)) / CLOCKS_PER_SEC;

    for (int i = 0; i < 1000; i++) {
        free(malloc_ptrs[i]);
    }

    printf("Memory pool time: %.6f seconds\n", pool_time);
    printf("Regular malloc time: %.6f seconds\n", malloc_time);
    printf("Pool usage: %zu bytes, Peak: %zu bytes\n",
           GetPoolUsage(pool), GetPoolPeakUsage(pool));

    FreeMemoryPool(pool);
    printf("Memory pool performance test passed!\n\n");
}

void test_string_buffer_performance() {
    printf("Testing string buffer performance...\n");

    clock_t start = clock();

    StringBuffer* buffer = CreateStringBuffer(1024);
    assert(buffer != NULL);

    for (int i = 0; i < 1000; i++) {
        char temp[32];
        snprintf(temp, sizeof(temp), "Line %d\n", i);
        assert(StringBufferAppend(buffer, temp));
    }

    clock_t end = clock();
    double buffer_time = ((double)(end - start)) / CLOCKS_PER_SEC;

    start = clock();
    char* str = malloc(1);
    str[0] = '\0';
    size_t str_len = 0;
    size_t str_cap = 1;

    for (int i = 0; i < 1000; i++) {
        char temp[32];
        snprintf(temp, sizeof(temp), "Line %d\n", i);
        size_t temp_len = strlen(temp);

        if (str_len + temp_len + 1 > str_cap) {
            str_cap = (str_len + temp_len + 1) * 2;
            str = realloc(str, str_cap);
            assert(str != NULL);
        }

        strcpy(str + str_len, temp);
        str_len += temp_len;
    }

    end = clock();
    double realloc_time = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("String buffer time: %.6f seconds\n", buffer_time);
    printf("Repeated realloc time: %.6f seconds\n", realloc_time);
    printf("String buffer length: %zu\n", StringBufferLength(buffer));

    free(str);
    FreeStringBuffer(buffer);
    printf("String buffer performance test passed!\n\n");
}

void test_memory_tracking() {
    printf("Testing memory tracking...\n");

    MemoryTracker* tracker = CreateMemoryTracker();
    assert(tracker != NULL);

    void* ptr1 = TRACKED_MALLOC(tracker, 100);
    void* ptr2 = TRACKED_MALLOC(tracker, 200);
    void* ptr3 = TRACKED_MALLOC(tracker, 300);

    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    assert(ptr3 != NULL);

    printf("After allocations:\n");
    PrintMemoryReport(tracker);

    TRACKED_FREE(tracker, ptr2);

    printf("After freeing ptr2:\n");
    PrintMemoryReport(tracker);

    ptr1 = TRACKED_REALLOC(tracker, ptr1, 150);
    assert(ptr1 != NULL);

    printf("After realloc:\n");
    PrintMemoryReport(tracker);

    TRACKED_FREE(tracker, ptr1);
    TRACKED_FREE(tracker, ptr3);

    printf("After freeing all:\n");
    PrintMemoryReport(tracker);

    assert(!HasMemoryLeaks(tracker));

    FreeMemoryTracker(tracker);
    printf("Memory tracking test passed!\n\n");
}

void test_http_client_memory_optimization() {
    printf("Testing HTTP client memory optimization...\n");

    assert(InitHttpClient());

    HttpRequest* request = CreateHttpRequest();
    assert(request != NULL);

    SetHttpRequestUrl(request, "https://httpbin.org/json");
    SetHttpRequestMethod(request, "GET");

    HttpResponse* response = SendHttpRequest(request);

    if (response) {
        printf("Response received: %d bytes\n", response->body ? (int)strlen(response->body) : 0);
        printf("Status code: %d\n", response->status_code);
        printf("Response time: %.2fms\n", response->response_time * 1000);

        FreeHttpResponse(response);
    } else {
        printf("Request failed (expected if libcurl not available)\n");
        HttpError error = GetLastHttpError();
        printf("Error: %s\n", error.message);
    }

    FreeHttpRequest(request);
    CleanupHttpClient();
    printf("HTTP client memory optimization test completed!\n\n");
}

void test_large_json_performance() {
    printf("Testing large JSON processing performance...\n");

    StringBuffer* json_buffer = CreateStringBuffer(100 * 1024);
    assert(json_buffer != NULL);

    StringBufferAppend(json_buffer, "{\n");
    StringBufferAppend(json_buffer, "  \"data\": [\n");

    for (int i = 0; i < 1000; i++) {
        char item[128];
        snprintf(item, sizeof(item),
                "    {\"id\": %d, \"name\": \"Item %d\", \"value\": %.2f}%s\n",
                i, i, (float)i * 1.5f, (i < 999) ? "," : "");
        StringBufferAppend(json_buffer, item);
    }

    StringBufferAppend(json_buffer, "  ]\n");
    StringBufferAppend(json_buffer, "}\n");

    const char* large_json = StringBufferData(json_buffer);
    printf("Generated JSON size: %zu bytes\n", strlen(large_json));

    clock_t start = clock();
    int is_valid = ValidateJson(large_json);
    clock_t end = clock();

    double validation_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("JSON validation time: %.6f seconds, valid: %s\n",
           validation_time, is_valid ? "yes" : "no");

    start = clock();
    char* formatted = FormatJson(large_json);
    end = clock();

    double format_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("JSON formatting time: %.6f seconds\n", format_time);

    if (formatted) {
        printf("Formatted JSON size: %zu bytes\n", strlen(formatted));
        free(formatted);
    }

    start = clock();
    int token_count = 0;
    JsonToken* tokens = TokenizeJson(large_json, &token_count);
    end = clock();

    double tokenize_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("JSON tokenization time: %.6f seconds, tokens: %d\n",
           tokenize_time, token_count);

    if (tokens) {
        FreeJsonTokens(tokens, token_count);
    }

    FreeStringBuffer(json_buffer);
    printf("Large JSON performance test completed!\n\n");
}

void test_ui_memory_optimization() {
    printf("Testing UI component memory optimization...\n");

    CleanupUIComponents();

    printf("UI memory optimization test completed!\n\n");
}

void run_performance_benchmark() {
    printf("Running performance benchmark...\n");

    clock_t total_start = clock();

    InitGlobalMemoryTracking();

    test_memory_pool_performance();
    test_string_buffer_performance();
    test_memory_tracking();
    test_http_client_memory_optimization();
    test_large_json_performance();
    test_ui_memory_optimization();

    clock_t total_end = clock();
    double total_time = ((double)(total_end - total_start)) / CLOCKS_PER_SEC;

    printf("Total benchmark time: %.6f seconds\n", total_time);

    CleanupGlobalMemoryTracking();

    printf("Performance benchmark completed!\n");
}

int main() {
    printf("=== Performance Optimization Tests ===\n\n");

    run_performance_benchmark();

    printf("\n=== All Performance Tests Passed! ===\n");
    return 0;
}