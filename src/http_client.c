/**
 * http client implementation using libcurl for tinyrequest
 *
 * this file provides a complete http client implementation that handles
 * all types of http requests with proper error handling, ssl support,
 * cookie management, and response processing. it wraps libcurl to provide
 * a simpler interface while maintaining full functionality.
 *
 * the client supports all standard http methods, custom headers, request
 * bodies, authentication, ssl certificate validation, progress callbacks,
 * and automatic cookie handling. it includes comprehensive error handling
 * and validation to ensure reliable network operations.
 *
 * response size limits and progress tracking help prevent memory issues
 * with large downloads while providing feedback to the user interface.
 */

#include "http_client.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <strings.h>
#include <sys/time.h>

/* global out-of-memory handler for http client */
static void (*g_http_client_out_of_memory_handler)(const char* operation) = NULL;

/* default out-of-memory handler */
static void default_http_client_out_of_memory_handler(const char* operation) {
    fprintf(stderr, "HTTP Client: Out of memory error during: %s\n", operation ? operation : "unknown operation");
    fflush(stderr);
}

/* helper function to handle memory allocation failures */
static void handle_http_client_out_of_memory(const char* operation) {
    if (g_http_client_out_of_memory_handler) {
        g_http_client_out_of_memory_handler(operation);
    } else {
        default_http_client_out_of_memory_handler(operation);
    }
}

/* creates a new http client with default configuration */
HttpClient* http_client_create(void) {
    /* initialize libcurl globally if not already done */
    static int curl_initialized = 0;
    if (!curl_initialized) {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
            return NULL;
        }
        curl_initialized = 1;
    }

    /* allocate memory for httpclient structure */
    HttpClient* client = (HttpClient*)malloc(sizeof(HttpClient));
    if (!client) {
        handle_http_client_out_of_memory("HTTP client creation");
        return NULL;
    }

    /* initialize the curl handle */
    client->curl_handle = curl_easy_init();
    if (!client->curl_handle) {
        free(client);
        return NULL;
    }

    /* clear the error buffer */
    memset(client->error_buffer, 0, CURL_ERROR_SIZE);

    /* initialize ssl verification settings (default to secure) */
    client->ssl_verify_peer = 1;
    client->ssl_verify_host = 2;

    /* initialize large response handling settings */
    client->max_response_size = 100 * 1024 * 1024; 
    client->current_response_size = 0;
    client->progress_callback = NULL;
    client->progress_userdata = NULL;

    /* set up basic libcurl configuration */
    curl_easy_setopt(client->curl_handle, CURLOPT_ERRORBUFFER, client->error_buffer);
    curl_easy_setopt(client->curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(client->curl_handle, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(client->curl_handle, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(client->curl_handle, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(client->curl_handle, CURLOPT_USERAGENT, "TinyRequest/1.0");

    /* ssl configuration - use client settings */
    curl_easy_setopt(client->curl_handle, CURLOPT_SSL_VERIFYPEER, (long)client->ssl_verify_peer);
    curl_easy_setopt(client->curl_handle, CURLOPT_SSL_VERIFYHOST, (long)client->ssl_verify_host);

    /* on linux, libcurl will use the system ca bundle automatically */

    /* set a reasonable ssl version range */
    curl_easy_setopt(client->curl_handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

    return client;
}

/* destroys an http client and frees all resources */
void http_client_destroy(HttpClient* client) {
    if (!client) {
        return;
    }

    /* clean up the curl handle */
    if (client->curl_handle) {
        curl_easy_cleanup(client->curl_handle);
        client->curl_handle = NULL;
    }

    /* free the client structure */
    free(client);
}

/* structure to pass both response and client to callback */
typedef struct {
    Response* response;
    HttpClient* client;
} ResponseCallbackData;

/* callback function to write response data */
static size_t write_response_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    if (!contents || !userp) {
        return 0; 
    }

    ResponseCallbackData* callback_data = (ResponseCallbackData*)userp;
    Response* response = callback_data->response;
    HttpClient* client = callback_data->client;

    if (!response || !client) {
        return 0; 
    }

    /* check for multiplication overflow */
    if (size > 0 && nmemb > SIZE_MAX / size) {
        return 0; 
    }

    size_t realsize = size * nmemb;

    /* use client's max response size setting */
    size_t max_size = client->max_response_size;

    /* check if we would exceed the maximum response size */
    if (response->body_size + realsize > max_size) {

        size_t remaining = max_size - response->body_size;

        if (remaining > 0) {

            realsize = remaining;
            response->is_truncated = 1;
        } else {

            response->is_truncated = 1;
            return 0;
        }
    }

    /* check for addition overflow */
    if (response->body_size > SIZE_MAX - realsize - 1) {
        return 0; 
    }

    /* reallocate memory for the response body with safety checks */
    char* new_body = realloc(response->body, response->body_size + realsize + 1);
    if (!new_body) {
        handle_http_client_out_of_memory("response body reallocation");

        return 0;
    }

    response->body = new_body;

    /* safe memory copy with bounds checking */
    if (realsize > 0) {
        memcpy(&(response->body[response->body_size]), contents, realsize);
        response->body_size += realsize;
        response->body[response->body_size] = '\0'; 

        client->current_response_size = response->body_size;
    }

    return realsize;
}

/* progress callback for libcurl */
static int tinyrequest_progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                        curl_off_t ultotal, curl_off_t ulnow) {
    (void)ultotal; /* unused parameter */
    (void)ulnow;   /* unused parameter */

    if (!clientp) {
        return 0; 
    }

    HttpClient* client = (HttpClient*)clientp;

    /* call user progress callback if set */
    if (client->progress_callback) {
        return client->progress_callback(client->progress_userdata, 
                                       (double)dltotal, (double)dlnow);
    }

    return 0; /* continue download */
}

/* callback function to write response headers */
static size_t write_header_callback(char* buffer, size_t size, size_t nitems, void* userp) {
    if (!buffer || !userp) {
        return 0; 
    }

    /* check for multiplication overflow */
    if (size > 0 && nitems > SIZE_MAX / size) {
        return 0; 
    }

    size_t realsize = size * nitems;
    Response* response = (Response*)userp;

    /* prevent processing excessively long headers */
    if (realsize > 8192) {
        return realsize; 
    }

    /* limit total number of headers to prevent memory exhaustion */
    if (response->headers.count >= 100) {
        return realsize; 
    }

    if (realsize > SIZE_MAX - 1) {
        return realsize; 
    }

    char* header_copy = malloc(realsize + 1);
    if (!header_copy) {
        handle_http_client_out_of_memory("header parsing buffer allocation");
        return realsize; 
    }

    memcpy(header_copy, buffer, realsize);
    header_copy[realsize] = '\0';

    /* parse header line safely */
    char* colon = strchr(header_copy, ':');
    if (colon && realsize > 2) { 
        *colon = '\0';
        char* name = header_copy;
        char* value = colon + 1;

        /* trim whitespace from value */
        while (*value == ' ' || *value == '\t') value++;

        size_t value_len = strlen(value);
        while (value_len > 0 && (value[value_len-1] == '\n' || value[value_len-1] == '\r' || 
                                value[value_len-1] == ' ' || value[value_len-1] == '\t')) {
            value[value_len-1] = '\0';
            value_len--;
        }

        size_t name_len = strlen(name);
        if (name_len > 0 && name_len < 128 && value_len < 512) {

            if (strcasecmp(name, "content-length") == 0) {
                response->total_size = (size_t)strtoul(value, NULL, 10);
            }

            header_list_add(&response->headers, name, value);
        }
    }

    free(header_copy);
    return realsize;
}

/* sends an http request and populates the response */
int http_client_send_request(HttpClient* client, const Request* request, Response* response) {
    if (!client || !client->curl_handle || !request || !response) {
        return -1; 
    }

    /* clear error buffer before each request */
    memset(client->error_buffer, 0, CURL_ERROR_SIZE);

    /* validate url before sending request */
    int url_validation = http_client_validate_url(request->url);
    if (url_validation != 0) {

        response_cleanup(response);
        response_init(response);
        response->status_code = 400; 

        switch (url_validation) {
            case -1:
                strncpy(response->status_text, "Empty URL", sizeof(response->status_text) - 1);
                break;
            case -2:
                strncpy(response->status_text, "URL Too Long", sizeof(response->status_text) - 1);
                break;
            case -3:
                strncpy(response->status_text, "Invalid Protocol", sizeof(response->status_text) - 1);
                break;
            case -4:
                strncpy(response->status_text, "No Domain", sizeof(response->status_text) - 1);
                break;
            case -5:
                strncpy(response->status_text, "Invalid Characters", sizeof(response->status_text) - 1);
                break;
            case -6:
                strncpy(response->status_text, "Domain Too Long", sizeof(response->status_text) - 1);
                break;
            case -7:
                strncpy(response->status_text, "Invalid Domain", sizeof(response->status_text) - 1);
                break;
            default:
                strncpy(response->status_text, "Invalid URL", sizeof(response->status_text) - 1);
        }
        response->status_text[sizeof(response->status_text) - 1] = '\0';
        return url_validation;
    }

    /* validate request method */
    if (strlen(request->method) == 0 || strlen(request->method) >= 16) {
        response_cleanup(response);
        response_init(response);
        response->status_code = 400;
        strncpy(response->status_text, "Invalid HTTP Method", sizeof(response->status_text) - 1);
        response->status_text[sizeof(response->status_text) - 1] = '\0';
        return -1;
    }

    /* reset response safely */
    response_cleanup(response);
    response_init(response);

    /* reset client's current response size */
    client->current_response_size = 0;

    CURL* curl = client->curl_handle;
    CURLcode res;
    struct curl_slist* headers = NULL;

    /* prepare callback data structure */
    ResponseCallbackData callback_data = {
        .response = response,
        .client = client
    };

    /* set url with additional safety */
    res = curl_easy_setopt(curl, CURLOPT_URL, request->url);
    if (res != CURLE_OK) {
        response->status_code = 0;
        strncpy(response->status_text, "Failed to set URL", sizeof(response->status_text) - 1);
        response->status_text[sizeof(response->status_text) - 1] = '\0';
        return -1;
    }

    /* apply current ssl settings */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)client->ssl_verify_peer);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, (long)client->ssl_verify_host);

    /* set http method with enhanced validation and safety checks */
    if (strcmp(request->method, "GET") == 0) {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    } else if (strcmp(request->method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (request->body && request->body_size > 0) {

            if (request->body_size > 50 * 1024 * 1024) {
                response->status_code = 413; 
                strncpy(response->status_text, "Request body too large", sizeof(response->status_text) - 1);
                response->status_text[sizeof(response->status_text) - 1] = '\0';
                return -1;
            }

            if (request->body[request->body_size - 1] != '\0' && request->body_size > 0) {

            }
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)request->body_size);
        } else {

            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
        }
    } else if (strcmp(request->method, "PUT") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        if (request->body && request->body_size > 0) {
            if (request->body_size > 50 * 1024 * 1024) {
                response->status_code = 413;
                strncpy(response->status_text, "Request body too large", sizeof(response->status_text) - 1);
                response->status_text[sizeof(response->status_text) - 1] = '\0';
                return -1;
            }
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)request->body_size);
        }
    } else if (strcmp(request->method, "DELETE") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        if (request->body && request->body_size > 0) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)request->body_size);
        }
    } else if (strcmp(request->method, "PATCH") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        if (request->body && request->body_size > 0) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)request->body_size);
        }
    } else if (strcmp(request->method, "HEAD") == 0) {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    } else if (strcmp(request->method, "OPTIONS") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
    } else {

        if (strlen(request->method) > 10) {
            response->status_code = 400;
            strncpy(response->status_text, "Invalid custom method", sizeof(response->status_text) - 1);
            response->status_text[sizeof(response->status_text) - 1] = '\0';
            return -1;
        }
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request->method);
    }

    /* set headers with error handling */
    headers = headers_to_curl_list(&request->headers);
    if (headers) {
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        if (res != CURLE_OK) {
            curl_slist_free_all(headers);
            response->status_code = 0;
            strncpy(response->status_text, "Failed to set headers", sizeof(response->status_text) - 1);
            response->status_text[sizeof(response->status_text) - 1] = '\0';
            return -1;
        }
    }

    /* set callback functions */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &callback_data);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, response);

    /* set progress callback if available */
    if (client->progress_callback) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, tinyrequest_progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, client);
    } else {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    }

    /* perform the request and measure wall-clock time using gettimeofday for better compatibility */
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    res = curl_easy_perform(curl);

    gettimeofday(&end_time, NULL);

    /* calculate elapsed time in milliseconds */
    double elapsed_seconds = (end_time.tv_sec - start_time.tv_sec) +
                            (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    response->response_time = elapsed_seconds * 1000.0; 

    /* get response code */
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    response->status_code = (int)response_code;

    /* set status text based on status code with better descriptions */
    if (res == CURLE_OK) {
        if (response->status_code >= 200 && response->status_code < 300) {
            strncpy(response->status_text, "OK", sizeof(response->status_text) - 1);
        } else if (response->status_code == 400) {
            strncpy(response->status_text, "Bad Request", sizeof(response->status_text) - 1);
        } else if (response->status_code == 401) {
            strncpy(response->status_text, "Unauthorized", sizeof(response->status_text) - 1);
        } else if (response->status_code == 403) {
            strncpy(response->status_text, "Forbidden", sizeof(response->status_text) - 1);
        } else if (response->status_code == 404) {
            strncpy(response->status_text, "Not Found", sizeof(response->status_text) - 1);
        } else if (response->status_code == 405) {
            strncpy(response->status_text, "Method Not Allowed", sizeof(response->status_text) - 1);
        } else if (response->status_code >= 400 && response->status_code < 500) {
            strncpy(response->status_text, "Client Error", sizeof(response->status_text) - 1);
        } else if (response->status_code == 500) {
            strncpy(response->status_text, "Internal Server Error", sizeof(response->status_text) - 1);
        } else if (response->status_code >= 500) {
            strncpy(response->status_text, "Server Error", sizeof(response->status_text) - 1);
        } else {
            strncpy(response->status_text, "Unknown", sizeof(response->status_text) - 1);
        }
        response->status_text[sizeof(response->status_text) - 1] = '\0';
    }

    /* handle curl errors with user-friendly messages */
    if (res != CURLE_OK) {

        response->status_code = 0; 

        switch (res) {
            case CURLE_COULDNT_RESOLVE_HOST:
                strncpy(response->status_text, "Could not resolve host", sizeof(response->status_text) - 1);
                break;
            case CURLE_COULDNT_CONNECT:
                strncpy(response->status_text, "Connection failed", sizeof(response->status_text) - 1);
                break;
            case CURLE_OPERATION_TIMEDOUT:
                strncpy(response->status_text, "Request timeout", sizeof(response->status_text) - 1);
                break;
            case CURLE_SSL_CONNECT_ERROR:
                strncpy(response->status_text, "SSL connection error", sizeof(response->status_text) - 1);
                break;
            case CURLE_SSL_PEER_CERTIFICATE:
                strncpy(response->status_text, "SSL certificate error", sizeof(response->status_text) - 1);
                break;
            case CURLE_TOO_MANY_REDIRECTS:
                strncpy(response->status_text, "Too many redirects", sizeof(response->status_text) - 1);
                break;
            case CURLE_URL_MALFORMAT:
                strncpy(response->status_text, "Malformed URL", sizeof(response->status_text) - 1);
                break;
            case CURLE_OUT_OF_MEMORY:
                strncpy(response->status_text, "Out of memory", sizeof(response->status_text) - 1);
                break;
            case CURLE_SEND_ERROR:
                strncpy(response->status_text, "Send error", sizeof(response->status_text) - 1);
                break;
            case CURLE_RECV_ERROR:
                strncpy(response->status_text, "Receive error", sizeof(response->status_text) - 1);
                break;
            case CURLE_HTTP_RETURNED_ERROR:
                strncpy(response->status_text, "HTTP error", sizeof(response->status_text) - 1);
                break;
            case CURLE_WRITE_ERROR:
                strncpy(response->status_text, "Write error", sizeof(response->status_text) - 1);
                break;
            default:
                snprintf(response->status_text, sizeof(response->status_text),
                        "Network error (%d)", res);
        }
        response->status_text[sizeof(response->status_text) - 1] = '\0';

        if (strlen(client->error_buffer) > 0) {
            size_t current_len = strlen(response->status_text);
            size_t available = sizeof(response->status_text) - current_len - 3; 

            if (available > 10) { 
                strncat(response->status_text, ": ", available);
                strncat(response->status_text, client->error_buffer, available - 2);
                response->status_text[sizeof(response->status_text) - 1] = '\0';
            }
        }
    }

    /* clean up headers */
    if (headers) {
        curl_slist_free_all(headers);
    }

    /* reset curl options for next request (but preserve our basic config) */
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, NULL);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 0L);
    curl_easy_setopt(curl, CURLOPT_POST, 0L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);

    return (res == CURLE_OK) ? 0 : -1;
}

/* sends an http request with automatic cookie handling */
int http_client_send_request_with_cookies(HttpClient* client, const Request* request, Response* response, Collection* collection) {
    if (!client || !client->curl_handle || !request || !response || !collection) {
        return -1; 
    }

    /* first, clean up expired cookies from the collection's cookie jar */
    cookie_jar_cleanup_expired(&collection->cookie_jar);

    /* check if the url uses https for secure cookie handling */
    bool is_secure = (strncmp(request->url, "https://", 8) == 0);

    /* build cookie header from the collection's cookie jar */
    printf("DEBUG: Building cookie header for URL: %s (secure: %s)\n", request->url, is_secure ? "yes" : "no");
    printf("DEBUG: Collection has %d cookies in jar\n", collection->cookie_jar.count);

    for (int i = 0; i < collection->cookie_jar.count; i++) {
        StoredCookie* cookie = &collection->cookie_jar.cookies[i];
        printf("DEBUG: Cookie %d: %s=%s (domain: %s, path: %s, secure: %s, expired: %s)\n",
               i, cookie->name, cookie->value, cookie->domain, cookie->path,
               cookie->secure ? "yes" : "no",
               cookie_jar_is_cookie_expired(cookie) ? "yes" : "no");
    }

    char* cookie_header = cookie_jar_build_cookie_header(&collection->cookie_jar, request->url, is_secure);

    if (cookie_header && strlen(cookie_header) > 0) {
        printf("DEBUG: Built cookie header: %s\n", cookie_header);
    } else {
        printf("DEBUG: No cookie header built (no matching cookies)\n");
    }

    /* create a modified request with the cookie header if we have cookies */
    Request modified_request = *request; 
    HeaderList modified_headers;
    header_list_init(&modified_headers);

    /* copy all existing headers */
    for (int i = 0; i < request->headers.count; i++) {
        header_list_add(&modified_headers, request->headers.headers[i].name, request->headers.headers[i].value);
    }

    /* add or update the cookie header if we have cookies to send */
    if (cookie_header && strlen(cookie_header) > 0) {

        bool cookie_header_found = false;
        for (int i = 0; i < modified_headers.count; i++) {
            if (strcasecmp(modified_headers.headers[i].name, "Cookie") == 0) {

                strncpy(modified_headers.headers[i].value, cookie_header, sizeof(modified_headers.headers[i].value) - 1);
                modified_headers.headers[i].value[sizeof(modified_headers.headers[i].value) - 1] = '\0';
                cookie_header_found = true;
                break;
            }
        }

        if (!cookie_header_found) {
            header_list_add(&modified_headers, "Cookie", cookie_header);
        }
    }

    /* update the modified request to use the new headers */
    modified_request.headers = modified_headers;

    /* send the request using the original function */
    int result = http_client_send_request(client, &modified_request, response);

    /* after receiving the response, process any set-cookie headers */
    if (result == 0 && response->headers.count > 0) {
        for (int i = 0; i < response->headers.count; i++) {
            if (strcasecmp(response->headers.headers[i].name, "Set-Cookie") == 0) {

                int cookie_result = cookie_jar_parse_set_cookie(&collection->cookie_jar,
                                                              response->headers.headers[i].value,
                                                              request->url);
                if (cookie_result >= 0) {

                    collection_update_modified_time(collection);
                }
            }
        }
    }

    /* clean up */
    if (cookie_header) {
        free(cookie_header);
    }
    header_list_cleanup(&modified_headers);

    return result;
}

/* configures ssl certificate verification settings */
void http_client_set_ssl_verification(HttpClient* client, int verify_peer, int verify_host) {
    if (!client || !client->curl_handle) {
        return;
    }

    client->ssl_verify_peer = verify_peer;
    client->ssl_verify_host = verify_host;

    /* update the curl handle with new ssl settings */
    curl_easy_setopt(client->curl_handle, CURLOPT_SSL_VERIFYPEER, (long)verify_peer);
    curl_easy_setopt(client->curl_handle, CURLOPT_SSL_VERIFYHOST, (long)verify_host);
}

/* validates a url for basic correctness and security */
int http_client_validate_url(const char* url) {
    if (!url || strlen(url) == 0) {
        return -1; 
    }

    /* check url length */
    if (strlen(url) >= 2048) {
        return -2; 
    }

    /* check for valid protocol */
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        return -3; 
    }

    /* find the start of the domain (after protocol) */
    const char* domain_start = strstr(url, "://");
    if (!domain_start) {
        return -3; 
    }
    domain_start += 3; 

    /* check if domain exists */
    if (strlen(domain_start) == 0) {
        return -4; 
    }

    /* check for invalid characters in url */
    const char* invalid_chars = " \t\n\r";
    for (int i = 0; invalid_chars[i]; i++) {
        if (strchr(url, invalid_chars[i])) {
            return -5; 
        }
    }

    /* basic domain validation - check for at least one dot or localhost */
    const char* domain_end = strchr(domain_start, '/');
    const char* port_start = strchr(domain_start, ':');

    /* determine domain length */
    size_t domain_len;
    if (domain_end && port_start && port_start < domain_end) {
        domain_len = port_start - domain_start;
    } else if (domain_end) {
        domain_len = domain_end - domain_start;
    } else if (port_start) {
        domain_len = port_start - domain_start;
    } else {
        domain_len = strlen(domain_start);
    }

    /* check for localhost or ip address patterns */
    if (strncmp(domain_start, "localhost", 9) == 0 ||
        strncmp(domain_start, "127.0.0.1", 9) == 0 ||
        strncmp(domain_start, "::1", 3) == 0) {
        return 0; 
    }

    /* check for at least one dot in domain (basic domain validation) */
    char domain_copy[256];
    if (domain_len >= sizeof(domain_copy)) {
        return -6; 
    }

    strncpy(domain_copy, domain_start, domain_len);
    domain_copy[domain_len] = '\0';

    if (!strchr(domain_copy, '.')) {
        return -7; 
    }

    return 0; 
}

/* converts a headerlist to curl's slist format */
struct curl_slist* headers_to_curl_list(const HeaderList* headers) {
    if (!headers || headers->count == 0) {
        return NULL;
    }

    struct curl_slist* curl_headers = NULL;

    for (int i = 0; i < headers->count; i++) {

        if (strlen(headers->headers[i].name) == 0) {
            continue; 
        }

        size_t name_len = strlen(headers->headers[i].name);
        size_t value_len = strlen(headers->headers[i].value);

        if (name_len == 0 || name_len >= 128 || value_len >= 512) {
            continue; 
        }

        char header_line[640]; 
        int result = snprintf(header_line, sizeof(header_line), "%s: %s",
                headers->headers[i].name, headers->headers[i].value);

        if (result < 0 || result >= (int)sizeof(header_line)) {
            continue; 
        }

        struct curl_slist* new_headers = curl_slist_append(curl_headers, header_line);
        if (!new_headers) {

            if (curl_headers) {
                curl_slist_free_all(curl_headers);
            }
            return NULL;
        }
        curl_headers = new_headers;
    }

    return curl_headers;
}

/* frees a curl slist */
void curl_list_free(struct curl_slist* list) {
    if (list) {
        curl_slist_free_all(list);
    }
}

/* sets the maximum response size limit */
void http_client_set_max_response_size(HttpClient* client, size_t max_size) {
    if (!client) {
        return;
    }

    /* set reasonable limits (minimum 1kb, maximum 1gb) */
    if (max_size < 1024) {
        max_size = 1024;
    } else if (max_size > 1024 * 1024 * 1024) {
        max_size = 1024 * 1024 * 1024;
    }

    client->max_response_size = max_size;
}

/* sets a progress callback for download tracking */
void http_client_set_progress_callback(HttpClient* client,
                                      int (*callback)(void* userdata, double total, double now),
                                      void* userdata) {
    if (!client) {
        return;
    }

    client->progress_callback = callback;
    client->progress_userdata = userdata;
}

/* sets a custom out-of-memory handler for http client operations */
void http_client_set_out_of_memory_handler(void (*handler)(const char* operation)) {
    g_http_client_out_of_memory_handler = handler;
}