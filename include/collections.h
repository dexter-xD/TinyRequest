/**
 * collections.h
 * 
 * request collections and cookie management for tinyrequest
 * 
 * this file handles organizing http requests into collections, which is
 * basically like folders for grouping related requests together. each
 * collection can have multiple requests, and each request has a name
 * so you can easily find what you're looking for.
 * 
 * it also handles cookie management - each collection has its own cookie
 * jar that automatically stores cookies from responses and sends them
 * back with future requests. this makes it easy to work with apis that
 * use session cookies or other stateful authentication.
 * 
 * the collection manager keeps track of all your collections and which
 * one is currently active. it's basically the organizational backbone
 * of the whole application.
 */

#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include <stddef.h>
#include <time.h>
#include <stdbool.h>
#include "request_response.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char name[128];
    char value[512];
    char domain[256];
    char path[256];
    time_t expires;
    int max_age;
    bool secure;
    bool http_only;
    bool same_site_strict;
    bool same_site_lax;
    time_t created_at;
} StoredCookie;

typedef struct {
    StoredCookie* cookies;
    int count;
    int capacity;
} CookieJar;

typedef struct {
    char id[64];
    char name[256];
    char description[512];
    Request* requests;
    char** request_names;
    int request_count;
    int request_capacity;
    time_t created_at;
    time_t modified_at;
    CookieJar cookie_jar;
} Collection;

typedef struct {
    Collection* collections;
    int count;
    int capacity;
    int active_collection_index;
    int active_request_index;
} CollectionManager;

Collection* collection_create(const char* name, const char* description);
void collection_destroy(Collection* collection);
void collection_init(Collection* collection, const char* name, const char* description);
void collection_cleanup(Collection* collection);

int collection_add_request(Collection* collection, const Request* request, const char* name);
int collection_remove_request(Collection* collection, int request_index);
int collection_duplicate_request(Collection* collection, int request_index);
int collection_rename_request(Collection* collection, int request_index, const char* new_name);
Request* collection_get_request(Collection* collection, int request_index);
const char* collection_get_request_name(Collection* collection, int request_index);

int collection_set_name(Collection* collection, const char* name);
int collection_set_description(Collection* collection, const char* description);
void collection_update_modified_time(Collection* collection);

CollectionManager* collection_manager_create(void);
void collection_manager_destroy(CollectionManager* manager);
void collection_manager_init(CollectionManager* manager);
void collection_manager_cleanup(CollectionManager* manager);

int collection_manager_add_collection(CollectionManager* manager, Collection* collection);
int collection_manager_remove_collection(CollectionManager* manager, int collection_index);
int collection_manager_duplicate_collection(CollectionManager* manager, int collection_index);
Collection* collection_manager_get_collection(CollectionManager* manager, int collection_index);

Collection* collection_manager_get_active_collection(CollectionManager* manager);
Request* collection_manager_get_active_request(CollectionManager* manager);
int collection_manager_set_active_collection(CollectionManager* manager, int collection_index);
int collection_manager_set_active_request(CollectionManager* manager, int request_index);

int collection_manager_find_collection_by_name(CollectionManager* manager, const char* name);
int collection_manager_get_total_requests(CollectionManager* manager);
bool collection_manager_has_collections(CollectionManager* manager);

CookieJar* cookie_jar_create(void);
void cookie_jar_destroy(CookieJar* jar);
void cookie_jar_init(CookieJar* jar);
void cookie_jar_cleanup(CookieJar* jar);
void stored_cookie_cleanup(StoredCookie* cookie);
int cookie_jar_add_cookie(CookieJar* jar, const char* name, const char* value,
                         const char* domain, const char* path, time_t expires, int max_age,
                         bool secure, bool http_only, bool same_site_strict, bool same_site_lax);
int cookie_jar_remove_cookie(CookieJar* jar, int cookie_index);
int cookie_jar_find_cookie(CookieJar* jar, const char* name, const char* domain, const char* path);
StoredCookie* cookie_jar_get_cookie(CookieJar* jar, int cookie_index);
bool cookie_jar_is_cookie_expired(const StoredCookie* cookie);
int cookie_jar_cleanup_expired(CookieJar* jar);
bool cookie_jar_matches_request(const StoredCookie* cookie, const char* url, bool is_secure);
int cookie_jar_get_cookies_for_request(CookieJar* jar, const char* url, bool is_secure,
                                      StoredCookie** matching_cookies, int max_cookies);
char* cookie_jar_build_cookie_header(CookieJar* jar, const char* url, bool is_secure);
void cookie_jar_clear_all(CookieJar* jar);
int cookie_jar_parse_set_cookie(CookieJar* jar, const char* set_cookie_header, const char* request_url);

bool collection_validate_name(const char* name);
bool collection_validate_description(const char* description);
bool collection_is_valid(const Collection* collection);

void collections_set_out_of_memory_handler(void (*handler)(const char* operation));

#ifdef __cplusplus
}
#endif

#endif