/*
 * request_response.h
 *
 * http request and response handling for tinyrequest
 *
 * this file contains all the data structures and functions for working with
 * http requests and responses. it handles the nitty-gritty details of managing
 * headers, request bodies, response data, and all the memory management that
 * goes along with it.
 *
 * the main structures are pretty straightforward - a request has a method,
 * url, headers, and optional body. a response has a status code, headers,
 * body, and some timing info. headers are stored in a dynamic list that
 * grows as needed.
 *
 * all the memory management is handled automatically, so you don't have to
 * worry about manually allocating and freeing memory for request/response
 * bodies or header lists.
 */

#ifndef REQUEST_RESPONSE_H
#define REQUEST_RESPONSE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* a single http header with name and value */
typedef struct {
  char name[128];
  char value[512];
} Header;

/* dynamic list of headers that grows as needed */
typedef struct {
  Header *headers;
  int count;
  int capacity;
} HeaderList;

/* complete http request with all the parts */
typedef struct {
  char method[16];    /* get, post, put, delete, etc. */
  char url[2048];     /* the full request url */
  HeaderList headers; /* custom headers */
  char *body;         /* request body (allocated dynamically) */
  size_t body_size;   /* size of the request body */

  /* per-request authentication data */
  int selected_auth_type; /* 0=None, 1=API Key, 2=Bearer Token, 3=Basic Auth,
                             4=OAuth 2.0 */
  char auth_api_key_name[128];   /* API Key name/header name */
  char auth_api_key_value[512];  /* API Key value */
  char auth_bearer_token[512];   /* Bearer token value */
  char auth_basic_username[256]; /* Basic auth username */
  char auth_basic_password[256]; /* Basic auth password */
  char auth_oauth_token[512];    /* OAuth 2.0 access token */
  int auth_api_key_location;     /* 0=Header, 1=Query Parameter */

  /* per-request authentication enable/disable checkboxes */
  bool auth_api_key_enabled; /* Whether to send API Key auth */
  bool auth_bearer_enabled;  /* Whether to send Bearer Token auth */
  bool auth_basic_enabled;   /* Whether to send Basic Auth */
  bool auth_oauth_enabled;   /* Whether to send OAuth 2.0 auth */
} Request;

/* complete http response with all the parts */
typedef struct {
  int status_code;      /* http status code like 200, 404, etc. */
  char status_text[64]; /* status message like "ok" or "not found" */
  HeaderList headers;   /* response headers */
  char *body;           /* response body (allocated dynamically) */
  size_t body_size;     /* size of the response body */
  double response_time; /* how long the request took in milliseconds */
  int is_truncated;     /* whether response was cut off due to size limits */
  size_t total_size;    /* total size if known from content-length header */
} Response;

/* error codes for when things go wrong */
typedef enum {
  REQUEST_RESPONSE_SUCCESS = 0,
  REQUEST_RESPONSE_ERROR_NULL_PARAM = -1,
  REQUEST_RESPONSE_ERROR_MEMORY_ALLOCATION = -2,
  REQUEST_RESPONSE_ERROR_INVALID_SIZE = -3,
  REQUEST_RESPONSE_ERROR_BUFFER_OVERFLOW = -4
} RequestResponseError;

/* creates a new empty header list */
HeaderList *header_list_create(void);

/* frees a header list and all its memory */
void header_list_destroy(HeaderList *list);

/* adds a new header to the list */
int header_list_add(HeaderList *list, const char *name, const char *value);

/* removes a header at the given index */
int header_list_remove(HeaderList *list, int index);

/* removes all headers from the list */
void header_list_clear(HeaderList *list);

/* finds a header by name, returns index or -1 if not found */
int header_list_find(HeaderList *list, const char *name);

/* updates an existing header or adds it if it doesn't exist */
int header_list_update(HeaderList *list, const char *name, const char *value);

/* checks if a header name is valid */
int header_validate_name(const char *name);

/* checks if a header value is valid */
int header_validate_value(const char *value);

/* creates a new empty request */
Request *request_create(void);

/* frees a request and all its memory */
void request_destroy(Request *request);

/* initializes a request struct to default values */
void request_init(Request *request);

/* cleans up a request's contents but doesn't free the struct itself */
void request_cleanup(Request *request);

/* sets the body content for a request */
int request_set_body(Request *request, const char *body, size_t size);

/* creates a new empty response */
Response *response_create(void);

/* frees a response and all its memory */
void response_destroy(Response *response);

/* initializes a response struct to default values */
void response_init(Response *response);

/* cleans up a response's contents but doesn't free the struct itself */
void response_cleanup(Response *response);

/* sets the body content for a response */
int response_set_body(Response *response, const char *body, size_t size);

/* converts an error code to a human-readable string */
const char *request_response_error_string(RequestResponseError error);

/* sets a custom handler for out-of-memory situations */
void request_response_set_out_of_memory_handler(
    void (*handler)(const char *operation));

void header_list_clear(HeaderList *list);
void header_list_init(HeaderList *list);
void header_list_cleanup(HeaderList *list);

#ifdef __cplusplus
}
#endif

#endif
