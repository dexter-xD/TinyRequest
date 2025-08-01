/**
 * http request and response data structures for tinyrequest
 *
 * this file implements the core data structures for handling http requests
 * and responses. it provides safe memory management, header manipulation,
 * and validation functions for http data.
 *
 * the request structure holds all the information needed to make an http
 * call - method, url, headers, and body content. the response structure
 * captures the server's reply including status codes, headers, response
 * body, and timing information.
 *
 * all functions include bounds checking and memory safety features to
 * prevent buffer overflows and memory leaks.
 */

#include "request_response.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/* global out-of-memory handler */
static void (*g_out_of_memory_handler)(const char* operation) = NULL;

/* default out-of-memory handler */
static void default_out_of_memory_handler(const char* operation) {
    fprintf(stderr, "Out of memory error during: %s\n", operation ? operation : "unknown operation");
    fflush(stderr);
}

/* helper function to handle memory allocation failures */
static void handle_out_of_memory(const char* operation) {
    if (g_out_of_memory_handler) {
        g_out_of_memory_handler(operation);
    } else {
        default_out_of_memory_handler(operation);
    }
}

/* creates a new header list structure */
HeaderList* header_list_create(void) {
    HeaderList* list = (HeaderList*)malloc(sizeof(HeaderList));
    if (list == NULL) {
        handle_out_of_memory("header list creation");
        return NULL;
    }
    
    list->headers = NULL;
    list->count = 0;
    list->capacity = 0;
    
    return list;
}

/* destroys a header list and frees its memory */
void header_list_destroy(HeaderList* list) {
    if (list == NULL) {
        return;
    }
    
    if (list->headers != NULL) {
        free(list->headers);
    }
    
    free(list);
}

/* adds a new header to the list with validation */
int header_list_add(HeaderList* list, const char* name, const char* value) {
    if (list == NULL || name == NULL || value == NULL) {
        return -1;
    }
    
    /* validate header name and value */
    if (header_validate_name(name) != 0 || header_validate_value(value) != 0) {
        return -1;
    }
    
    /* check if we need to expand capacity */
    if (list->count >= list->capacity) {
        int new_capacity = list->capacity == 0 ? 4 : list->capacity * 2;
        
        // Prevent integer overflow and excessive memory allocation
        if (new_capacity < list->capacity || new_capacity > 1000) {
            return -1; // Capacity overflow or too large
        }
        
        Header* new_headers = (Header*)realloc(list->headers, new_capacity * sizeof(Header));
        if (new_headers == NULL) {
            handle_out_of_memory("header list expansion");
            return REQUEST_RESPONSE_ERROR_MEMORY_ALLOCATION;
        }
        list->headers = new_headers;
        list->capacity = new_capacity;
    }
    
    /* add the header with safe string copying */
    size_t name_len = strlen(name);
    size_t value_len = strlen(value);
    
    // Double-check lengths to prevent buffer overflow
    if (name_len >= sizeof(list->headers[list->count].name) ||
        value_len >= sizeof(list->headers[list->count].value)) {
        return -1; // String too long
    }
    
    memcpy(list->headers[list->count].name, name, name_len);
    list->headers[list->count].name[name_len] = '\0';
    
    memcpy(list->headers[list->count].value, value, value_len);
    list->headers[list->count].value[value_len] = '\0';
    
    list->count++;
    return 0;
}

/* removes a header from the list by index */
int header_list_remove(HeaderList* list, int index) {
    if (list == NULL || index < 0 || index >= list->count) {
        return -1;
    }
    
    /* shift remaining headers down */
    for (int i = index; i < list->count - 1; i++) {
        list->headers[i] = list->headers[i + 1];
    }
    
    list->count--;
    return 0;
}

/* clears all headers from the list */
void header_list_clear(HeaderList* list) {
    if (list == NULL) {
        return;
    }
    
    if (list->headers != NULL) {
        free(list->headers);
        list->headers = NULL;
    }
    
    list->count = 0;
    list->capacity = 0;
}

/* initializes a header list structure (for stack-allocated headerlist) */
void header_list_init(HeaderList* list) {
    if (list == NULL) {
        return;
    }
    
    list->headers = NULL;
    list->count = 0;
    list->capacity = 0;
}

/* cleans up a header list structure (for stack-allocated headerlist) */
void header_list_cleanup(HeaderList* list) {
    if (list == NULL) {
        return;
    }
    
    if (list->headers != NULL) {
        free(list->headers);
        list->headers = NULL;
    }
    
    list->count = 0;
    list->capacity = 0;
}

/* creates a new request structure */
Request* request_create(void) {
    Request* request = (Request*)malloc(sizeof(Request));
    if (request == NULL) {
        handle_out_of_memory("request creation");
        return NULL;
    }
    request_init(request);
    return request;
}

/* destroys a request structure and frees its memory */
void request_destroy(Request* request) {
    if (request == NULL) {
        return;
    }
    request_cleanup(request);
    free(request);
}

/* initializes a request structure with default values */
void request_init(Request* request) {
    if (request == NULL) {
        return;
    }
    
    /* initialize method to get by default */
    strncpy(request->method, "GET", sizeof(request->method) - 1);
    request->method[sizeof(request->method) - 1] = '\0';
    
    /* initialize url to empty string */
    request->url[0] = '\0';
    
    /* initialize headers list */
    request->headers.headers = NULL;
    request->headers.count = 0;
    request->headers.capacity = 0;
    
    /* initialize body */
    request->body = NULL;
    request->body_size = 0;
    
    /* initialize authentication data */
    request->selected_auth_type = 0;
    memset(request->auth_api_key_name, 0, sizeof(request->auth_api_key_name));
    memset(request->auth_api_key_value, 0, sizeof(request->auth_api_key_value));
    memset(request->auth_bearer_token, 0, sizeof(request->auth_bearer_token));
    memset(request->auth_basic_username, 0, sizeof(request->auth_basic_username));
    memset(request->auth_basic_password, 0, sizeof(request->auth_basic_password));
    memset(request->auth_oauth_token, 0, sizeof(request->auth_oauth_token));
    request->auth_api_key_location = 0;
    
    /* initialize authentication checkboxes (enabled by default for backward compatibility) */
    request->auth_api_key_enabled = true;
    request->auth_bearer_enabled = true;
    request->auth_basic_enabled = true;
    request->auth_oauth_enabled = true;
}

/* cleans up a request structure and frees its resources */
void request_cleanup(Request* request) {
    if (request == NULL) {
        return;
    }
    
    /* free headers array */
    if (request->headers.headers != NULL) {
        free(request->headers.headers);
        request->headers.headers = NULL;
    }
    request->headers.count = 0;
    request->headers.capacity = 0;
    
    /* free body */
    if (request->body != NULL) {
        free(request->body);
        request->body = NULL;
    }
    request->body_size = 0;
}

/* sets the request body with size validation and memory management */
int request_set_body(Request* request, const char* body, size_t size) {
    if (request == NULL) {
        return -1;
    }
    
    /* free existing body */
    if (request->body != NULL) {
        free(request->body);
        request->body = NULL;
        request->body_size = 0;
    }
    
    /* if body is null or size is 0, just clear the body */
    if (body == NULL || size == 0) {
        return 0;
    }
    
    /* prevent excessive memory allocation (limit to 50mb) */
    if (size > 50 * 1024 * 1024) {
        return -1; // Body too large
    }
    
    /* allocate new body with overflow check */
    if (size > SIZE_MAX - 1) {
        return -1; // Size overflow
    }
    
    request->body = (char*)malloc(size + 1);
    if (request->body == NULL) {
        handle_out_of_memory("request body allocation");
        return REQUEST_RESPONSE_ERROR_MEMORY_ALLOCATION;
    }
    
    /* copy body content safely and null-terminate */
    memcpy(request->body, body, size);
    request->body[size] = '\0';
    request->body_size = size;
    
    return 0;
}

/* creates a new response structure */
Response* response_create(void) {
    Response* response = (Response*)malloc(sizeof(Response));
    if (response == NULL) {
        handle_out_of_memory("response creation");
        return NULL;
    }
    response_init(response);
    return response;
}

/* destroys a response structure and frees its memory */
void response_destroy(Response* response) {
    if (response == NULL) {
        return;
    }
    response_cleanup(response);
    free(response);
}

/* initializes a response structure with default values */
void response_init(Response* response) {
    if (response == NULL) {
        return;
    }
    
    /* initialize status */
    response->status_code = 0;
    response->status_text[0] = '\0';
    
    /* initialize headers list */
    response->headers.headers = NULL;
    response->headers.count = 0;
    response->headers.capacity = 0;
    
    /* initialize body */
    response->body = NULL;
    response->body_size = 0;
    
    /* initialize timing */
    response->response_time = 0.0;
    
    /* initialize large response handling fields */
    response->is_truncated = 0;
    response->total_size = 0;
}

/* cleans up a response structure and frees its resources */
void response_cleanup(Response* response) {
    if (response == NULL) {
        return;
    }
    
    /* free headers array */
    if (response->headers.headers != NULL) {
        free(response->headers.headers);
        response->headers.headers = NULL;
    }
    response->headers.count = 0;
    response->headers.capacity = 0;
    
    /* free body */
    if (response->body != NULL) {
        free(response->body);
        response->body = NULL;
    }
    response->body_size = 0;
}

/* sets the response body with size validation and memory management */
int response_set_body(Response* response, const char* body, size_t size) {
    if (response == NULL) {
        return -1;
    }
    
    /* free existing body */
    if (response->body != NULL) {
        free(response->body);
        response->body = NULL;
        response->body_size = 0;
    }
    
    /* if body is null or size is 0, just clear the body */
    if (body == NULL || size == 0) {
        return 0;
    }
    
    /* prevent excessive memory allocation (limit to 100mb for responses) */
    if (size > 100 * 1024 * 1024) {
        return -1; // Response body too large
    }
    
    /* allocate new body with overflow check */
    if (size > SIZE_MAX - 1) {
        return -1; // Size overflow
    }
    
    response->body = (char*)malloc(size + 1);
    if (response->body == NULL) {
        handle_out_of_memory("response body allocation");
        return REQUEST_RESPONSE_ERROR_MEMORY_ALLOCATION;
    }
    
    /* copy body content safely and null-terminate */
    memcpy(response->body, body, size);
    response->body[size] = '\0';
    response->body_size = size;
    
    return 0;
}



/* finds a header by name and returns its index */
int header_list_find(HeaderList* list, const char* name) {
    if (list == NULL || name == NULL) {
        return -1;
    }
    
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->headers[i].name, name) == 0) {
            return i;
        }
    }
    
    return -1;
}

/* updates an existing header or adds a new one */
int header_list_update(HeaderList* list, const char* name, const char* value) {
    if (list == NULL || name == NULL || value == NULL) {
        return -1;
    }
    
    int index = header_list_find(list, name);
    if (index >= 0) {
        // Update existing header
        if (header_validate_value(value) != 0) {
            return -1;
        }
        strncpy(list->headers[index].value, value, sizeof(list->headers[index].value) - 1);
        list->headers[index].value[sizeof(list->headers[index].value) - 1] = '\0';
        return 0;
    } else {
        // Add new header
        return header_list_add(list, name, value);
    }
}

/* validates a header name according to http standards */
int header_validate_name(const char* name) {
    if (name == NULL || strlen(name) == 0) {
        return -1;
    }
    
    /* check length */
    if (strlen(name) >= sizeof(((Header*)0)->name)) {
        return -1;
    }
    
    /* check for invalid characters */
    for (const char* p = name; *p; p++) {
        if (*p == ':' || *p == '\r' || *p == '\n' || *p == ' ' || *p == '\t') {
            return -1;
        }
        // Header names should be printable ASCII
        if (*p < 33 || *p > 126) {
            return -1;
        }
    }
    
    return 0;
}

/* validates a header value according to http standards */
int header_validate_value(const char* value) {
    if (value == NULL) {
        return -1;
    }
    
    /* check length */
    if (strlen(value) >= sizeof(((Header*)0)->value)) {
        return -1;
    }
    
    /* check for invalid characters (no cr/lf) */
    for (const char* p = value; *p; p++) {
        if (*p == '\r' || *p == '\n') {
            return -1;
        }
    }
    
    return 0;
}
/* returns a human-readable error message for the given error code */
const char* request_response_error_string(RequestResponseError error) {
    switch (error) {
        case REQUEST_RESPONSE_SUCCESS:
            return "Success";
        case REQUEST_RESPONSE_ERROR_NULL_PARAM:
            return "Null parameter provided";
        case REQUEST_RESPONSE_ERROR_MEMORY_ALLOCATION:
            return "Memory allocation failed";
        case REQUEST_RESPONSE_ERROR_INVALID_SIZE:
            return "Invalid size parameter";
        case REQUEST_RESPONSE_ERROR_BUFFER_OVERFLOW:
            return "Buffer overflow prevented";
        default:
            return "Unknown error";
    }
}

/* sets a custom out-of-memory handler for request/response operations */
void request_response_set_out_of_memory_handler(void (*handler)(const char* operation)) {
    g_out_of_memory_handler = handler;
}