#include "request_state.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

RequestState* CreateRequestState(void) {
    RequestState* state = malloc(sizeof(RequestState));
    if (!state) {
        return NULL;
    }

    state->selected_method = HTTP_GET;
    memset(state->url, 0, MAX_URL_LENGTH);
    memset(state->body, 0, MAX_BODY_LENGTH);
    memset(state->headers, 0, sizeof(state->headers));
    state->header_count = 0;
    state->last_response = NULL;
    state->request_in_progress = false;
    state->last_request_time = 0.0;
    memset(state->status_message, 0, sizeof(state->status_message));

    strcpy(state->status_message, "Ready");

    return state;
}

void FreeRequestState(RequestState* state) {
    if (!state) {
        return;
    }

    if (state->last_response) {
        FreeHttpResponse(state->last_response);
        state->last_response = NULL;
    }

    free(state);
}

void ResetRequestState(RequestState* state) {
    if (!state) {
        return;
    }

    if (state->last_response) {
        FreeHttpResponse(state->last_response);
        state->last_response = NULL;
    }

    state->selected_method = HTTP_GET;
    memset(state->url, 0, MAX_URL_LENGTH);
    memset(state->body, 0, MAX_BODY_LENGTH);
    memset(state->headers, 0, sizeof(state->headers));
    state->header_count = 0;
    state->request_in_progress = false;
    state->last_request_time = 0.0;
    strcpy(state->status_message, "Ready");
}

int SaveRequestState(RequestState* state, const char* filename) {
    if (!state || !filename) {
        return -1;
    }

    FILE* file = fopen(filename, "w");
    if (!file) {
        return -1;
    }

    fprintf(file, "method=%d\n", state->selected_method);
    fprintf(file, "url=%s\n", state->url);
    fprintf(file, "body=%s\n", state->body);
    fprintf(file, "header_count=%d\n", state->header_count);

    for (int i = 0; i < state->header_count && i < MAX_HEADERS; i++) {
        fprintf(file, "header_%d=%s\n", i, state->headers[i]);
    }

    fclose(file);
    return 0;
}

int LoadRequestState(RequestState* state, const char* filename) {
    if (!state || !filename) {
        return -1;
    }

    FILE* file = fopen(filename, "r");
    if (!file) {
        return -1;
    }

    char line[1024];
    char key[256];
    char value[768];

    ResetRequestState(state);

    while (fgets(line, sizeof(line), file)) {

        line[strcspn(line, "\n")] = 0;

        char* equals_pos = strchr(line, '=');
        if (!equals_pos) {
            continue;
        }

        size_t key_len = equals_pos - line;
        if (key_len >= sizeof(key)) {
            continue;
        }

        strncpy(key, line, key_len);
        key[key_len] = '\0';
        strcpy(value, equals_pos + 1);

        if (strcmp(key, "method") == 0) {
            state->selected_method = atoi(value);
            if (state->selected_method < HTTP_GET || state->selected_method > HTTP_DELETE) {
                state->selected_method = HTTP_GET;
            }
        } else if (strcmp(key, "url") == 0) {
            strncpy(state->url, value, MAX_URL_LENGTH - 1);
            state->url[MAX_URL_LENGTH - 1] = '\0';
        } else if (strcmp(key, "body") == 0) {
            strncpy(state->body, value, MAX_BODY_LENGTH - 1);
            state->body[MAX_BODY_LENGTH - 1] = '\0';
        } else if (strcmp(key, "header_count") == 0) {
            state->header_count = atoi(value);
            if (state->header_count < 0 || state->header_count > MAX_HEADERS) {
                state->header_count = 0;
            }
        } else if (strncmp(key, "header_", 7) == 0) {
            int header_index = atoi(key + 7);
            if (header_index >= 0 && header_index < MAX_HEADERS) {
                strncpy(state->headers[header_index], value, MAX_HEADER_LENGTH - 1);
                state->headers[header_index][MAX_HEADER_LENGTH - 1] = '\0';
            }
        }
    }

    fclose(file);
    return 0;
}

HttpRequest* BuildHttpRequestFromState(RequestState* state) {
    if (!state) {
        return NULL;
    }

    HttpRequest* request = CreateHttpRequest();
    if (!request) {
        return NULL;
    }

    if (strlen(state->url) > 0) {
        SetHttpRequestUrl(request, state->url);
    }

    const char* method_string = GetHttpMethodString(state->selected_method);
    SetHttpRequestMethod(request, method_string);

    if (strlen(state->body) > 0) {
        SetHttpRequestBody(request, state->body);
    }

    for (int i = 0; i < state->header_count && i < MAX_HEADERS; i++) {
        if (strlen(state->headers[i]) > 0) {

            char* colon_pos = strchr(state->headers[i], ':');
            if (colon_pos) {

                size_t key_len = colon_pos - state->headers[i];
                char* key = malloc(key_len + 1);
                if (key) {
                    strncpy(key, state->headers[i], key_len);
                    key[key_len] = '\0';

                    char* value = colon_pos + 1;
                    while (*value == ' ' || *value == '\t') {
                        value++;
                    }

                    AddHttpRequestHeader(request, key, value);
                    free(key);
                }
            }
        }
    }

    return request;
}

void UpdateStateFromResponse(RequestState* state, HttpResponse* response) {
    if (!state) {
        return;
    }

    if (state->last_response) {
        FreeHttpResponse(state->last_response);
    }

    state->last_response = response;
    state->request_in_progress = false;

    if (response) {
        state->last_request_time = response->response_time;

        if (response->status_code >= 200 && response->status_code < 300) {
            snprintf(state->status_message, sizeof(state->status_message),
                    "Success: %d (%.2fms)", response->status_code, response->response_time * 1000);
        } else if (response->status_code >= 400) {
            snprintf(state->status_message, sizeof(state->status_message),
                    "Error: %d (%.2fms)", response->status_code, response->response_time * 1000);
        } else {
            snprintf(state->status_message, sizeof(state->status_message),
                    "Response: %d (%.2fms)", response->status_code, response->response_time * 1000);
        }
    } else {
        strcpy(state->status_message, "Request failed");
    }
}

bool IsValidRequestState(RequestState* state) {
    if (!state) {
        return false;
    }

    if (strlen(state->url) == 0) {
        return false;
    }

    if (ValidateUrl(state->url) != 0) {
        return false;
    }

    if (state->selected_method < HTTP_GET || state->selected_method > HTTP_DELETE) {
        return false;
    }

    if (state->header_count < 0 || state->header_count > MAX_HEADERS) {
        return false;
    }

    for (int i = 0; i < state->header_count; i++) {
        if (strlen(state->headers[i]) > 0) {
            if (!strchr(state->headers[i], ':')) {
                return false;
            }
        }
    }

    return true;
}

char* GetStateValidationError(RequestState* state) {
    if (!state) {
        return strdup("Invalid state: NULL pointer");
    }

    if (strlen(state->url) == 0) {
        return strdup("URL is required");
    }

    if (ValidateUrl(state->url) != 0) {
        return strdup("Invalid URL format");
    }

    if (state->selected_method < HTTP_GET || state->selected_method > HTTP_DELETE) {
        return strdup("Invalid HTTP method");
    }

    if (state->header_count < 0 || state->header_count > MAX_HEADERS) {
        return strdup("Invalid header count");
    }

    for (int i = 0; i < state->header_count; i++) {
        if (strlen(state->headers[i]) > 0) {
            if (!strchr(state->headers[i], ':')) {
                char* error = malloc(256);
                if (error) {
                    snprintf(error, 256, "Invalid header format at index %d: missing colon", i);
                    return error;
                }
                return strdup("Invalid header format");
            }
        }
    }

    return NULL;
}

const char* GetHttpMethodString(HttpMethod method) {
    switch (method) {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_PUT: return "PUT";
        case HTTP_DELETE: return "DELETE";
        default: return "GET";
    }
}

HttpMethod GetHttpMethodFromString(const char* method_string) {
    if (!method_string) {
        return HTTP_GET;
    }

    if (strcmp(method_string, "POST") == 0) return HTTP_POST;
    if (strcmp(method_string, "PUT") == 0) return HTTP_PUT;
    if (strcmp(method_string, "DELETE") == 0) return HTTP_DELETE;
    return HTTP_GET;
}

void SetStatusMessage(RequestState* state, const char* message) {
    if (!state || !message) {
        return;
    }

    strncpy(state->status_message, message, sizeof(state->status_message) - 1);
    state->status_message[sizeof(state->status_message) - 1] = '\0';
}

const char* GetDefaultStateFilePath(void) {
    static char path[512];
    const char* home = getenv("USERPROFILE");
    if (!home) {
        home = getenv("HOME");
    }

    if (home) {
        snprintf(path, sizeof(path), "%s/.tinyrequest_state", home);
    } else {
        strcpy(path, "tinyrequest_state.txt");
    }

    return path;
}

int SaveDefaultState(RequestState* state) {
    return SaveRequestState(state, GetDefaultStateFilePath());
}

int LoadDefaultState(RequestState* state) {
    return LoadRequestState(state, GetDefaultStateFilePath());
}

void ClearRequestHistory(RequestState* state) {
    if (!state) {
        return;
    }

    if (state->last_response) {
        FreeHttpResponse(state->last_response);
        state->last_response = NULL;
    }

    state->last_request_time = 0.0;
    strcpy(state->status_message, "Ready");
}