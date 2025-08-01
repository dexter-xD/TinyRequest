/**
 * http_client.h
 * 
 * http networking and request execution for tinyrequest
 * 
 * this file wraps libcurl to handle actually sending http requests
 * and receiving responses. it manages ssl settings, handles large
 * responses that might need to be truncated, and provides progress
 * callbacks for long-running requests.
 * 
 * also integrates with the collections system to automatically
 * handle cookies - storing them from responses and sending them
 * back with future requests to the same domain.
 * 
 * basically the networking engine that makes all the http magic
 * happen behind the scenes.
 */

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <curl/curl.h>
#include "request_response.h"
#include "collections.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    CURL* curl_handle;
    char error_buffer[CURL_ERROR_SIZE];
    int ssl_verify_peer;
    int ssl_verify_host;
    size_t max_response_size;
    size_t current_response_size;
    int (*progress_callback)(void* userdata, double download_total, double download_now);
    void* progress_userdata;
} HttpClient;

HttpClient* http_client_create(void);
void http_client_destroy(HttpClient* client);

int http_client_send_request(HttpClient* client, const Request* request, Response* response);
int http_client_send_request_with_cookies(HttpClient* client, const Request* request, Response* response, Collection* collection);

void http_client_set_ssl_verification(HttpClient* client, int verify_peer, int verify_host);

void http_client_set_max_response_size(HttpClient* client, size_t max_size);
void http_client_set_progress_callback(HttpClient* client, 
                                      int (*callback)(void* userdata, double total, double now),
                                      void* userdata);

int http_client_validate_url(const char* url);

struct curl_slist* headers_to_curl_list(const HeaderList* headers);
void curl_list_free(struct curl_slist* list);

void http_client_set_out_of_memory_handler(void (*handler)(const char* operation));

#ifdef __cplusplus
}
#endif

#endif