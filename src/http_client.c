#include "http_client.h"
#include "memory_manager.h"
#include "raylib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef CURL_CURL_H
#include <curl/curl.h>
#endif

static int curl_initialized = 0;

static HttpError last_error = {HTTP_ERROR_NONE, ""};

void SetHttpError(HttpErrorCode code, const char* message) {
    last_error.code = code;
    if (message) {
        strncpy(last_error.message, message, sizeof(last_error.message) - 1);
        last_error.message[sizeof(last_error.message) - 1] = '\0';
    } else {
        last_error.message[0] = '\0';
    }
}

typedef struct {
    StringBuffer* buffer;
    size_t max_size;
} ResponseBuffer;

typedef struct {
    char** headers;
    int count;
    int capacity;
} HeaderBuffer;

int InitHttpClient(void) {
    if (curl_initialized) {
        return 1;
    }

#ifdef CURL_CURL_H
    CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (result != CURLE_OK) {
        return 0;
    }
#endif

    curl_initialized = 1;
    return 1;
}

void CleanupHttpClient(void) {
    if (curl_initialized) {
#ifdef CURL_CURL_H
        curl_global_cleanup();
#endif
        curl_initialized = 0;
    }
}

int ValidateUrl(const char* url) {
    if (!url || strlen(url) == 0) {
        return 0;
    }

    if (strlen(url) < 7) {
        return 0;
    }

    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        return 0;
    }

    const char* domain_start = strstr(url, "://");
    if (!domain_start) {
        return 0;
    }
    domain_start += 3;

    if (strlen(domain_start) == 0) {
        return 0;
    }

    if (*domain_start == '/') {
        return 0;
    }

    const char* domain_end = strchr(domain_start, '/');
    const char* port_start = strchr(domain_start, ':');

    if (port_start && domain_end && port_start > domain_end) {
        port_start = NULL;
    }

    if (*domain_start == '.' || *domain_start == '-') {
        return 0;
    }

    return 1;
}

HttpRequest* CreateHttpRequest(void) {
    HttpRequest* request = malloc(sizeof(HttpRequest));
    if (!request) {
        return NULL;
    }

    request->url = NULL;
    request->method = NULL;
    request->body = NULL;
    request->headers = NULL;
    request->header_count = 0;

    return request;
}

void SetHttpRequestUrl(HttpRequest* request, const char* url) {
    if (!request || !url) {
        return;
    }

    if (request->url) {
        free(request->url);
    }

    request->url = malloc(strlen(url) + 1);
    if (request->url) {
        strcpy(request->url, url);
    }
}

void SetHttpRequestMethod(HttpRequest* request, const char* method) {
    if (!request || !method) {
        return;
    }

    if (request->method) {
        free(request->method);
    }

    request->method = malloc(strlen(method) + 1);
    if (request->method) {
        strcpy(request->method, method);
    }
}

void SetHttpRequestBody(HttpRequest* request, const char* body) {
    if (!request || !body) {
        return;
    }

    if (request->body) {
        free(request->body);
    }

    request->body = malloc(strlen(body) + 1);
    if (request->body) {
        strcpy(request->body, body);
    }
}

void AddHttpRequestHeader(HttpRequest* request, const char* key, const char* value) {
    if (!request || !key || !value) {
        return;
    }

    char** new_headers = realloc(request->headers, (request->header_count + 1) * sizeof(char*));
    if (!new_headers) {
        return;
    }

    request->headers = new_headers;

    size_t header_len = strlen(key) + strlen(value) + 3;
    request->headers[request->header_count] = malloc(header_len);
    if (request->headers[request->header_count]) {
        snprintf(request->headers[request->header_count], header_len, "%s: %s", key, value);
        request->header_count++;
    }
}

void FreeHttpRequest(HttpRequest* request) {
    if (!request) {
        return;
    }

    if (request->url) {
        free(request->url);
    }

    if (request->method) {
        free(request->method);
    }

    if (request->body) {
        free(request->body);
    }

    if (request->headers) {
        for (int i = 0; i < request->header_count; i++) {
            if (request->headers[i]) {
                free(request->headers[i]);
            }
        }
        free(request->headers);
    }

    free(request);
}

static size_t WriteResponseCallback(void* contents, size_t size, size_t nmemb, ResponseBuffer* buffer) {
    size_t realsize = size * nmemb;

    if (buffer->max_size > 0 && StringBufferLength(buffer->buffer) + realsize > buffer->max_size) {
        return 0;
    }

    if (!buffer->buffer) {
        buffer->buffer = CreateStringBuffer(realsize > 8192 ? realsize : 8192);
        if (!buffer->buffer) {
            return 0;
        }
    }

    char* temp_str = malloc(realsize + 1);
    if (!temp_str) {
        return 0;
    }

    memcpy(temp_str, contents, realsize);
    temp_str[realsize] = '\0';

    bool success = StringBufferAppend(buffer->buffer, temp_str);
    free(temp_str);

    return success ? realsize : 0;
}

static size_t WriteHeaderCallback(void* contents, size_t size, size_t nmemb, HeaderBuffer* buffer) {
    size_t realsize = size * nmemb;

    if (realsize <= 2 || strncmp((char*)contents, "HTTP/", 5) == 0) {
        return realsize;
    }

    if (buffer->count >= buffer->capacity) {
        int new_capacity = buffer->capacity == 0 ? 10 : buffer->capacity * 2;
        char** new_headers = realloc(buffer->headers, new_capacity * sizeof(char*));
        if (!new_headers) {
            return 0;
        }
        buffer->headers = new_headers;
        buffer->capacity = new_capacity;
    }

    buffer->headers[buffer->count] = malloc(realsize + 1);
    if (!buffer->headers[buffer->count]) {
        return 0;
    }

    memcpy(buffer->headers[buffer->count], contents, realsize);
    buffer->headers[buffer->count][realsize] = 0;

    char* header = buffer->headers[buffer->count];
    size_t len = strlen(header);
    while (len > 0 && (header[len-1] == '\r' || header[len-1] == '\n')) {
        header[len-1] = 0;
        len--;
    }

    if (strlen(header) > 0) {
        buffer->count++;
    } else {
        free(buffer->headers[buffer->count]);
    }

    return realsize;
}

HttpResponse* SendHttpRequest(HttpRequest* request) {
    printf("DEBUG: SendHttpRequest called\n");

    ClearHttpError();

    if (!request || !request->url) {
        printf("DEBUG: Request or URL is NULL\n");
        SetHttpError(HTTP_ERROR_INVALID_URL, "Request or URL is NULL");
        return NULL;
    }

    printf("DEBUG: Request URL: %s\n", request->url);

    if (!ValidateUrl(request->url)) {
        printf("DEBUG: URL validation failed\n");
        SetHttpError(HTTP_ERROR_INVALID_URL, "Invalid URL format");
        return NULL;
    }

    printf("DEBUG: URL validation passed\n");

#ifndef CURL_CURL_H
    printf("DEBUG: CURL_CURL_H not defined - libcurl not available\n");
    SetHttpError(HTTP_ERROR_NETWORK_FAILURE, "libcurl not available");

    return NULL;
#else

    CURL* curl = curl_easy_init();
    if (!curl) {
        return NULL;
    }

    ResponseBuffer response_buffer = {0};
    response_buffer.max_size = 50 * 1024 * 1024;
    HeaderBuffer header_buffer = {0};

    HttpResponse* response = malloc(sizeof(HttpResponse));
    if (!response) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    response->status_code = 0;
    response->body = NULL;
    response->headers = NULL;
    response->header_count = 0;
    response->response_time = 0.0;

    curl_easy_setopt(curl, CURLOPT_URL, request->url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteResponseCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_buffer);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    if (request->method) {
        if (strcmp(request->method, "POST") == 0) {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        } else if (strcmp(request->method, "PUT") == 0) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        } else if (strcmp(request->method, "DELETE") == 0) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }

    }

    if (request->body && strlen(request->body) > 0) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(request->body));
    }

    struct curl_slist* headers = NULL;
    if (request->headers && request->header_count > 0) {
        for (int i = 0; i < request->header_count; i++) {
            if (request->headers[i]) {
                headers = curl_slist_append(headers, request->headers[i]);
            }
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    double start_time = GetTime();

    CURLcode res = curl_easy_perform(curl);

    double end_time = GetTime();
    response->response_time = (end_time - start_time) * 1000.0;

    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    response->status_code = (int)response_code;

    if (res != CURLE_OK) {

        HttpErrorCode error_code = HTTP_ERROR_UNKNOWN;
        char error_msg[256];

        switch (res) {
            case CURLE_COULDNT_CONNECT:
            case CURLE_COULDNT_RESOLVE_HOST:
            case CURLE_COULDNT_RESOLVE_PROXY:
                error_code = HTTP_ERROR_NETWORK_FAILURE;
                snprintf(error_msg, sizeof(error_msg), "Network connection failed: %s", curl_easy_strerror(res));
                break;
            case CURLE_OPERATION_TIMEDOUT:
                error_code = HTTP_ERROR_TIMEOUT;
                snprintf(error_msg, sizeof(error_msg), "Request timeout: %s", curl_easy_strerror(res));
                break;
            case CURLE_SSL_CONNECT_ERROR:
            case CURLE_SSL_CERTPROBLEM:
            case CURLE_SSL_CIPHER:
            case CURLE_SSL_CACERT:
                error_code = HTTP_ERROR_SSL_ERROR;
                snprintf(error_msg, sizeof(error_msg), "SSL/TLS error: %s", curl_easy_strerror(res));
                break;
            case CURLE_OUT_OF_MEMORY:
                error_code = HTTP_ERROR_OUT_OF_MEMORY;
                snprintf(error_msg, sizeof(error_msg), "Out of memory: %s", curl_easy_strerror(res));
                break;
            default:
                error_code = HTTP_ERROR_UNKNOWN;
                snprintf(error_msg, sizeof(error_msg), "HTTP request failed: %s", curl_easy_strerror(res));
                break;
        }

        SetHttpError(error_code, error_msg);

        if (response_buffer.buffer) {
            FreeStringBuffer(response_buffer.buffer);
        }
        if (header_buffer.headers) {
            for (int i = 0; i < header_buffer.count; i++) {
                free(header_buffer.headers[i]);
            }
            free(header_buffer.headers);
        }
        free(response);

        if (headers) {
            curl_slist_free_all(headers);
        }
        curl_easy_cleanup(curl);

        return NULL;
    }

    if (response_buffer.buffer) {
        response->body = StringBufferDetach(response_buffer.buffer);

        if (response->body) {
            size_t body_len = strlen(response->body);
            printf("DEBUG: Response body length: %zu characters\n", body_len);
            if (body_len > 0) {
                printf("DEBUG: Response body: %.100s", response->body);
                if (body_len > 100) {
                    printf("...");
                }
                printf("\n");

                if (body_len > 50) {
                    printf("DEBUG: Last 50 chars: ...%s\n", response->body + body_len - 50);
                } else {
                    printf("DEBUG: Complete response: %s\n", response->body);
                }

                if (body_len > 0) {
                    char* trimmed = response->body;
                    while (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n') trimmed++;

                    if (*trimmed == '{' || *trimmed == '[') {

                        int brace_count = 0;
                        int bracket_count = 0;
                        bool in_string = false;
                        bool escaped = false;

                        for (size_t i = 0; i < body_len; i++) {
                            char c = response->body[i];

                            if (in_string) {
                                if (escaped) {
                                    escaped = false;
                                } else if (c == '\\') {
                                    escaped = true;
                                } else if (c == '"') {
                                    in_string = false;
                                }
                            } else {
                                if (c == '"') {
                                    in_string = true;
                                } else if (c == '{') {
                                    brace_count++;
                                } else if (c == '}') {
                                    brace_count--;
                                } else if (c == '[') {
                                    bracket_count++;
                                } else if (c == ']') {
                                    bracket_count--;
                                }
                            }
                        }

                        if (brace_count > 0 || bracket_count > 0) {
                            printf("DEBUG: JSON appears incomplete (braces: %d, brackets: %d), attempting to fix\n", brace_count, bracket_count);

                            size_t fix_len = brace_count + bracket_count;
                            char* fixed_json = malloc(body_len + fix_len + 1);
                            if (fixed_json) {
                                strcpy(fixed_json, response->body);

                                for (int i = 0; i < bracket_count; i++) {
                                    strcat(fixed_json, "]");
                                }
                                for (int i = 0; i < brace_count; i++) {
                                    strcat(fixed_json, "}");
                                }

                                printf("DEBUG: Fixed JSON: %s\n", fixed_json);

                                free(response->body);
                                response->body = fixed_json;
                            }
                        }
                    }
                }
            }
        }

        FreeStringBuffer(response_buffer.buffer);
    } else {
        response->body = NULL;
    }

    response->headers = header_buffer.headers;
    response->header_count = header_buffer.count;

    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    return response;
#endif
}

void FreeHttpResponse(HttpResponse* response) {
    if (!response) {
        return;
    }

    if (response->body) {
        free(response->body);
    }

    if (response->headers) {
        for (int i = 0; i < response->header_count; i++) {
            if (response->headers[i]) {
                free(response->headers[i]);
            }
        }
        free(response->headers);
    }

    free(response);
}

AsyncHttpHandle* SendHttpRequestAsync(HttpRequest* request) {
    printf("DEBUG: SendHttpRequestAsync called\n");

    if (!request || !request->url) {
        printf("DEBUG: SendHttpRequestAsync - Request or URL is NULL\n");
        SetHttpError(HTTP_ERROR_INVALID_URL, "Request or URL is NULL");
        return NULL;
    }

    printf("DEBUG: SendHttpRequestAsync - URL: %s\n", request->url);

    if (!ValidateUrl(request->url)) {
        printf("DEBUG: SendHttpRequestAsync - URL validation failed\n");
        SetHttpError(HTTP_ERROR_INVALID_URL, "Invalid URL format");
        return NULL;
    }

    printf("DEBUG: SendHttpRequestAsync - URL validation passed\n");

    AsyncHttpHandle* handle = malloc(sizeof(AsyncHttpHandle));
    if (!handle) {
        printf("DEBUG: SendHttpRequestAsync - Failed to allocate async handle\n");
        SetHttpError(HTTP_ERROR_OUT_OF_MEMORY, "Failed to allocate async handle");
        return NULL;
    }

    printf("DEBUG: SendHttpRequestAsync - Async handle created\n");

    handle->curl_handle = NULL;
    handle->request = request;
    handle->response = NULL;
    handle->state = ASYNC_REQUEST_PENDING;
    handle->error_message[0] = '\0';
    handle->start_time = (double)clock() / CLOCKS_PER_SEC;
    handle->cancelled = 0;

#ifndef CURL_CURL_H
    printf("DEBUG: SendHttpRequestAsync - CURL_CURL_H not defined, marking as error\n");

    handle->state = ASYNC_REQUEST_ERROR;
    strcpy(handle->error_message, "libcurl not available");
    SetHttpError(HTTP_ERROR_NETWORK_FAILURE, "libcurl not available");
    return handle;
#else

    handle->response = SendHttpRequest(request);

    if (handle->response) {
        handle->state = ASYNC_REQUEST_COMPLETED;
    } else {
        handle->state = ASYNC_REQUEST_ERROR;

        HttpError last_err = GetLastHttpError();
        if (last_err.code != HTTP_ERROR_NONE) {
            strncpy(handle->error_message, last_err.message, sizeof(handle->error_message) - 1);
            handle->error_message[sizeof(handle->error_message) - 1] = '\0';
        } else {
            strcpy(handle->error_message, "HTTP request failed");
        }
    }

    return handle;
#endif
}

AsyncRequestState CheckAsyncRequest(AsyncHttpHandle* handle) {
    if (!handle) {
        return ASYNC_REQUEST_ERROR;
    }

    if (handle->cancelled && handle->state == ASYNC_REQUEST_PENDING) {
        handle->state = ASYNC_REQUEST_CANCELLED;
        strcpy(handle->error_message, "Request cancelled by user");
        return handle->state;
    }

    double current_time = (double)clock() / CLOCKS_PER_SEC;
    if (handle->state == ASYNC_REQUEST_PENDING &&
        (current_time - handle->start_time) > 30.0) {
        handle->state = ASYNC_REQUEST_TIMEOUT;
        strcpy(handle->error_message, "Request timeout after 30 seconds");
        SetHttpError(HTTP_ERROR_TIMEOUT, "Request timeout after 30 seconds");
    }

    return handle->state;
}

HttpResponse* GetAsyncResponse(AsyncHttpHandle* handle) {
    if (!handle || handle->state != ASYNC_REQUEST_COMPLETED) {
        return NULL;
    }

    return handle->response;
}

void CancelAsyncRequest(AsyncHttpHandle* handle) {
    if (!handle) {
        return;
    }

    handle->cancelled = 1;

    if (handle->state == ASYNC_REQUEST_PENDING) {
        handle->state = ASYNC_REQUEST_CANCELLED;
        strcpy(handle->error_message, "Request cancelled by user");
    }

#ifdef CURL_CURL_H

#endif
}

void FreeAsyncHandle(AsyncHttpHandle* handle) {
    if (!handle) {
        return;
    }

    free(handle);
}

HttpError GetLastHttpError(void) {
    return last_error;
}

const char* GetHttpErrorString(HttpErrorCode code) {
    switch (code) {
        case HTTP_ERROR_NONE:
            return "No error";
        case HTTP_ERROR_INVALID_URL:
            return "Invalid URL format";
        case HTTP_ERROR_NETWORK_FAILURE:
            return "Network connection failed";
        case HTTP_ERROR_TIMEOUT:
            return "Request timeout";
        case HTTP_ERROR_SSL_ERROR:
            return "SSL/TLS error";
        case HTTP_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case HTTP_ERROR_CANCELLED:
            return "Request cancelled";
        case HTTP_ERROR_UNKNOWN:
        default:
            return "Unknown error";
    }
}

void ClearHttpError(void) {
    last_error.code = HTTP_ERROR_NONE;
    last_error.message[0] = '\0';
}