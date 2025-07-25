#ifndef REQUEST_STATE_H
#define REQUEST_STATE_H

#include "http_client.h"
#include <stdbool.h>

#define MAX_URL_LENGTH 512
#define MAX_BODY_LENGTH 4096
#define MAX_HEADER_LENGTH 256
#define MAX_HEADERS 10

typedef enum {
    HTTP_GET = 0,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE
} HttpMethod;

typedef struct {
    int selected_method;
    char url[MAX_URL_LENGTH];
    char body[MAX_BODY_LENGTH];
    char headers[MAX_HEADERS][MAX_HEADER_LENGTH];
    int header_count;
    HttpResponse* last_response;
    bool request_in_progress;
    double last_request_time;
    char status_message[256];
} RequestState;

RequestState* CreateRequestState(void);
void FreeRequestState(RequestState* state);
void ResetRequestState(RequestState* state);

int SaveRequestState(RequestState* state, const char* filename);
int LoadRequestState(RequestState* state, const char* filename);

HttpRequest* BuildHttpRequestFromState(RequestState* state);
void UpdateStateFromResponse(RequestState* state, HttpResponse* response);

bool IsValidRequestState(RequestState* state);
char* GetStateValidationError(RequestState* state);

const char* GetHttpMethodString(HttpMethod method);
HttpMethod GetHttpMethodFromString(const char* method_string);
void SetStatusMessage(RequestState* state, const char* message);

const char* GetDefaultStateFilePath(void);
int SaveDefaultState(RequestState* state);
int LoadDefaultState(RequestState* state);
void ClearRequestHistory(RequestState* state);

#endif