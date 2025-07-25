#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/http_client.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        printf("Running test: %s\n", #name); \
        tests_run++; \
        if (name()) { \
            tests_passed++; \
            printf("  PASSED\n"); \
        } else { \
            printf("  FAILED\n"); \
        } \
    } while(0)

int test_validate_url_valid() {

    if (!ValidateUrl("http://example.com")) return 0;
    if (!ValidateUrl("https://example.com")) return 0;
    if (!ValidateUrl("http://www.example.com")) return 0;
    if (!ValidateUrl("https://www.example.com")) return 0;

    if (!ValidateUrl("http://example.com/path")) return 0;
    if (!ValidateUrl("https://example.com/path/to/resource")) return 0;

    if (!ValidateUrl("http://example.com:8080")) return 0;
    if (!ValidateUrl("https://example.com:443")) return 0;
    if (!ValidateUrl("http://localhost:3000")) return 0;

    if (!ValidateUrl("http://example.com/search?q=test")) return 0;
    if (!ValidateUrl("https://api.example.com/v1/users?limit=10")) return 0;

    return 1;
}

int test_validate_url_invalid() {

    if (ValidateUrl(NULL)) return 0;
    if (ValidateUrl("")) return 0;

    if (ValidateUrl("example.com")) return 0;
    if (ValidateUrl("www.example.com")) return 0;

    if (ValidateUrl("ftp://example.com")) return 0;
    if (ValidateUrl("file://example.com")) return 0;

    if (ValidateUrl("http://")) return 0;
    if (ValidateUrl("https://")) return 0;
    if (ValidateUrl("http:///path")) return 0;

    if (ValidateUrl("http://.example.com")) return 0;
    if (ValidateUrl("http://-example.com")) return 0;

    if (ValidateUrl("http:")) return 0;
    if (ValidateUrl("h")) return 0;

    return 1;
}

int test_http_client_init_cleanup() {

    if (!InitHttpClient()) return 0;

    if (!InitHttpClient()) return 0;

    CleanupHttpClient();

    if (!InitHttpClient()) return 0;
    CleanupHttpClient();

    return 1;
}

int test_http_request_creation() {
    HttpRequest* request = CreateHttpRequest();
    if (!request) return 0;

    if (request->url != NULL) return 0;
    if (request->method != NULL) return 0;
    if (request->body != NULL) return 0;
    if (request->headers != NULL) return 0;
    if (request->header_count != 0) return 0;

    FreeHttpRequest(request);
    return 1;
}

int test_http_request_url() {
    HttpRequest* request = CreateHttpRequest();
    if (!request) return 0;

    SetHttpRequestUrl(request, "https://api.example.com");
    if (!request->url || strcmp(request->url, "https://api.example.com") != 0) {
        FreeHttpRequest(request);
        return 0;
    }

    SetHttpRequestUrl(request, "http://localhost:8080");
    if (!request->url || strcmp(request->url, "http://localhost:8080") != 0) {
        FreeHttpRequest(request);
        return 0;
    }

    SetHttpRequestUrl(request, NULL);

    FreeHttpRequest(request);
    return 1;
}

int test_http_request_method() {
    HttpRequest* request = CreateHttpRequest();
    if (!request) return 0;

    SetHttpRequestMethod(request, "GET");
    if (!request->method || strcmp(request->method, "GET") != 0) {
        FreeHttpRequest(request);
        return 0;
    }

    SetHttpRequestMethod(request, "POST");
    if (!request->method || strcmp(request->method, "POST") != 0) {
        FreeHttpRequest(request);
        return 0;
    }

    FreeHttpRequest(request);
    return 1;
}

int test_http_request_body() {
    HttpRequest* request = CreateHttpRequest();
    if (!request) return 0;

    const char* json_body = "{\"name\":\"test\",\"value\":123}";
    SetHttpRequestBody(request, json_body);
    if (!request->body || strcmp(request->body, json_body) != 0) {
        FreeHttpRequest(request);
        return 0;
    }

    const char* new_body = "plain text body";
    SetHttpRequestBody(request, new_body);
    if (!request->body || strcmp(request->body, new_body) != 0) {
        FreeHttpRequest(request);
        return 0;
    }

    FreeHttpRequest(request);
    return 1;
}

int test_http_request_headers() {
    HttpRequest* request = CreateHttpRequest();
    if (!request) return 0;

    AddHttpRequestHeader(request, "Content-Type", "application/json");
    if (request->header_count != 1) {
        FreeHttpRequest(request);
        return 0;
    }
    if (!request->headers[0] || strcmp(request->headers[0], "Content-Type: application/json") != 0) {
        FreeHttpRequest(request);
        return 0;
    }

    AddHttpRequestHeader(request, "Authorization", "Bearer token123");
    if (request->header_count != 2) {
        FreeHttpRequest(request);
        return 0;
    }
    if (!request->headers[1] || strcmp(request->headers[1], "Authorization: Bearer token123") != 0) {
        FreeHttpRequest(request);
        return 0;
    }

    AddHttpRequestHeader(request, "User-Agent", "TinyRequest/1.0");
    if (request->header_count != 3) {
        FreeHttpRequest(request);
        return 0;
    }

    FreeHttpRequest(request);
    return 1;
}

int test_null_parameter_handling() {

    SetHttpRequestUrl(NULL, "http://example.com");
    SetHttpRequestMethod(NULL, "GET");
    SetHttpRequestBody(NULL, "body");
    AddHttpRequestHeader(NULL, "key", "value");

    HttpRequest* request = CreateHttpRequest();
    if (!request) return 0;

    AddHttpRequestHeader(request, NULL, "value");
    AddHttpRequestHeader(request, "key", NULL);
    AddHttpRequestHeader(request, NULL, NULL);

    if (request->header_count != 0) {
        FreeHttpRequest(request);
        return 0;
    }

    FreeHttpRequest(NULL);

    FreeHttpRequest(request);
    return 1;
}

int main() {
    printf("Running HTTP Client Tests\n");
    printf("========================\n");

    if (!InitHttpClient()) {
        printf("Failed to initialize HTTP client\n");
        return 1;
    }

    TEST(test_validate_url_valid);
    TEST(test_validate_url_invalid);
    TEST(test_http_client_init_cleanup);
    TEST(test_http_request_creation);
    TEST(test_http_request_url);
    TEST(test_http_request_method);
    TEST(test_http_request_body);
    TEST(test_http_request_headers);
    TEST(test_null_parameter_handling);

    CleanupHttpClient();

    printf("\n========================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);

    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
        return 0;
    } else {
        printf("Some tests FAILED!\n");
        return 1;
    }
}