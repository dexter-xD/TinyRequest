#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#ifndef CURL_CURL_H
typedef void CURL;
typedef int CURLcode;
#define CURLE_OK 0
#define CURL_GLOBAL_DEFAULT 0
#else
#include <curl/curl.h>
#endif

typedef struct {
    char* url;
    char* method;
    char* body;
    char** headers;
    int header_count;
} HttpRequest;

typedef struct {
    int status_code;
    char* body;
    char** headers;
    int header_count;
    double response_time;
} HttpResponse;

typedef enum {
    ASYNC_REQUEST_PENDING = 0,
    ASYNC_REQUEST_COMPLETED,
    ASYNC_REQUEST_ERROR,
    ASYNC_REQUEST_TIMEOUT,
    ASYNC_REQUEST_CANCELLED
} AsyncRequestState;

typedef struct {
    void* curl_handle;
    HttpRequest* request;
    HttpResponse* response;
    AsyncRequestState state;
    char error_message[256];
    double start_time;
    int cancelled;
} AsyncHttpHandle;

HttpResponse* SendHttpRequest(HttpRequest* request);
void FreeHttpResponse(HttpResponse* response);
void FreeHttpRequest(HttpRequest* request);

AsyncHttpHandle* SendHttpRequestAsync(HttpRequest* request);
AsyncRequestState CheckAsyncRequest(AsyncHttpHandle* handle);
HttpResponse* GetAsyncResponse(AsyncHttpHandle* handle);
void CancelAsyncRequest(AsyncHttpHandle* handle);
void FreeAsyncHandle(AsyncHttpHandle* handle);

int ValidateUrl(const char* url);
HttpRequest* CreateHttpRequest(void);
void SetHttpRequestUrl(HttpRequest* request, const char* url);
void SetHttpRequestMethod(HttpRequest* request, const char* method);
void SetHttpRequestBody(HttpRequest* request, const char* body);
void AddHttpRequestHeader(HttpRequest* request, const char* key, const char* value);

typedef enum {
    HTTP_ERROR_NONE = 0,
    HTTP_ERROR_INVALID_URL,
    HTTP_ERROR_NETWORK_FAILURE,
    HTTP_ERROR_TIMEOUT,
    HTTP_ERROR_SSL_ERROR,
    HTTP_ERROR_OUT_OF_MEMORY,
    HTTP_ERROR_CANCELLED,
    HTTP_ERROR_UNKNOWN
} HttpErrorCode;

typedef struct {
    HttpErrorCode code;
    char message[256];
} HttpError;

HttpError GetLastHttpError(void);
const char* GetHttpErrorString(HttpErrorCode code);
void ClearHttpError(void);
void SetHttpError(HttpErrorCode code, const char* message);

int InitHttpClient(void);
void CleanupHttpClient(void);

#endif