#include "collections.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

static void (*g_collections_out_of_memory_handler)(const char* operation) = NULL;

static void default_collections_out_of_memory_handler(const char* operation) {
    fprintf(stderr, "Collections: Out of memory error during: %s\n", operation ? operation : "unknown operation");
    fflush(stderr);
}

static void handle_collections_out_of_memory(const char* operation) {
    if (g_collections_out_of_memory_handler) {
        g_collections_out_of_memory_handler(operation);
    } else {
        default_collections_out_of_memory_handler(operation);
    }
}

#define DEFAULT_COLLECTION_CAPACITY 8
#define DEFAULT_REQUEST_CAPACITY 16
#define DEFAULT_COOKIE_CAPACITY 32

static void generate_collection_id(char* id_buffer, size_t buffer_size) {

    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }

    time_t now = time(NULL);
    int random_part = rand() % 10000;
    snprintf(id_buffer, buffer_size, "col_%ld_%d", now, random_part);
}

static int resize_array(void** array, int* capacity, size_t element_size) {
    if (!array || !capacity || *capacity < 0) {
        return -1; 
    }

    if (*capacity > 10000 || *capacity > INT_MAX / 2) {
        return -1; 
    }

    int new_capacity = (*capacity) * 2;

    if (element_size > SIZE_MAX / new_capacity) {
        return -1; 
    }

    void* new_array = realloc(*array, new_capacity * element_size);
    if (!new_array) {
        return -1; 
    }
    *array = new_array;
    *capacity = new_capacity;
    return 0;
}

Collection* collection_create(const char* name, const char* description) {
    Collection* collection = malloc(sizeof(Collection));
    if (!collection) {
        handle_collections_out_of_memory("collection creation");
        return NULL;
    }

    collection_init(collection, name, description);
    return collection;
}

void collection_destroy(Collection* collection) {
    if (!collection) {
        return;
    }

    collection_cleanup(collection);
    free(collection);
}

void collection_init(Collection* collection, const char* name, const char* description) {
    if (!collection) {
        return;
    }

    generate_collection_id(collection->id, sizeof(collection->id));

    const char* safe_name = name ? name : "Untitled Collection";
    const char* safe_description = description ? description : "";

    size_t name_len = strlen(safe_name);
    size_t desc_len = strlen(safe_description);

    if (name_len >= sizeof(collection->name)) {
        name_len = sizeof(collection->name) - 1;
    }
    if (desc_len >= sizeof(collection->description)) {
        desc_len = sizeof(collection->description) - 1;
    }

    memcpy(collection->name, safe_name, name_len);
    collection->name[name_len] = '\0';

    memcpy(collection->description, safe_description, desc_len);
    collection->description[desc_len] = '\0';

    collection->requests = malloc(DEFAULT_REQUEST_CAPACITY * sizeof(Request));
    collection->request_names = malloc(DEFAULT_REQUEST_CAPACITY * sizeof(char*));

    if (!collection->requests || !collection->request_names) {

        if (collection->requests) {
            free(collection->requests);
            collection->requests = NULL;
        }
        if (collection->request_names) {
            free(collection->request_names);
            collection->request_names = NULL;
        }
        collection->request_count = 0;
        collection->request_capacity = 0;
        return;
    }

    collection->request_count = 0;
    collection->request_capacity = DEFAULT_REQUEST_CAPACITY;

    time_t now = time(NULL);
    collection->created_at = now;
    collection->modified_at = now;

    for (int i = 0; i < DEFAULT_REQUEST_CAPACITY; i++) {
        collection->request_names[i] = NULL;
    }

    cookie_jar_init(&collection->cookie_jar);
}

void collection_cleanup(Collection* collection) {
    if (!collection) {
        return;
    }

    for (int i = 0; i < collection->request_count; i++) {
        request_cleanup(&collection->requests[i]);
        if (collection->request_names[i]) {
            free(collection->request_names[i]);
        }
    }

    cookie_jar_cleanup(&collection->cookie_jar);

    free(collection->requests);
    free(collection->request_names);

    collection->requests = NULL;
    collection->request_names = NULL;
    collection->request_count = 0;
    collection->request_capacity = 0;
}

int collection_add_request(Collection* collection, const Request* request, const char* name) {
    if (!collection || !request || !name) {
        return -1;
    }

    size_t name_len = strlen(name);
    if (name_len == 0 || name_len > 255) {
        return -1; 
    }

    if (collection->request_count >= collection->request_capacity) {
        if (resize_array((void**)&collection->requests, &collection->request_capacity, sizeof(Request)) != 0) {
            return -1;
        }

        char** new_names = realloc(collection->request_names, collection->request_capacity * sizeof(char*));
        if (!new_names) {
            return -1;
        }
        collection->request_names = new_names;

        for (int i = collection->request_count; i < collection->request_capacity; i++) {
            collection->request_names[i] = NULL;
        }
    }

    int index = collection->request_count;
    request_init(&collection->requests[index]);

    size_t method_len = strlen(request->method);
    size_t url_len = strlen(request->url);

    if (method_len >= sizeof(collection->requests[index].method)) {
        method_len = sizeof(collection->requests[index].method) - 1;
    }
    if (url_len >= sizeof(collection->requests[index].url)) {
        url_len = sizeof(collection->requests[index].url) - 1;
    }

    memcpy(collection->requests[index].method, request->method, method_len);
    collection->requests[index].method[method_len] = '\0';

    memcpy(collection->requests[index].url, request->url, url_len);
    collection->requests[index].url[url_len] = '\0';

    for (int i = 0; i < request->headers.count; i++) {
        if (header_list_add(&collection->requests[index].headers, 
                           request->headers.headers[i].name, 
                           request->headers.headers[i].value) != 0) {

        }
    }

    if (request->body && request->body_size > 0) {
        if (request_set_body(&collection->requests[index], request->body, request->body_size) != 0) {

        }
    }
    
    // Copy authentication data from source request
    collection->requests[index].selected_auth_type = request->selected_auth_type;
    strncpy(collection->requests[index].auth_api_key_name, request->auth_api_key_name, sizeof(collection->requests[index].auth_api_key_name) - 1);
    collection->requests[index].auth_api_key_name[sizeof(collection->requests[index].auth_api_key_name) - 1] = '\0';
    strncpy(collection->requests[index].auth_api_key_value, request->auth_api_key_value, sizeof(collection->requests[index].auth_api_key_value) - 1);
    collection->requests[index].auth_api_key_value[sizeof(collection->requests[index].auth_api_key_value) - 1] = '\0';
    strncpy(collection->requests[index].auth_bearer_token, request->auth_bearer_token, sizeof(collection->requests[index].auth_bearer_token) - 1);
    collection->requests[index].auth_bearer_token[sizeof(collection->requests[index].auth_bearer_token) - 1] = '\0';
    strncpy(collection->requests[index].auth_basic_username, request->auth_basic_username, sizeof(collection->requests[index].auth_basic_username) - 1);
    collection->requests[index].auth_basic_username[sizeof(collection->requests[index].auth_basic_username) - 1] = '\0';
    strncpy(collection->requests[index].auth_basic_password, request->auth_basic_password, sizeof(collection->requests[index].auth_basic_password) - 1);
    collection->requests[index].auth_basic_password[sizeof(collection->requests[index].auth_basic_password) - 1] = '\0';
    strncpy(collection->requests[index].auth_oauth_token, request->auth_oauth_token, sizeof(collection->requests[index].auth_oauth_token) - 1);
    collection->requests[index].auth_oauth_token[sizeof(collection->requests[index].auth_oauth_token) - 1] = '\0';
    collection->requests[index].auth_api_key_location = request->auth_api_key_location;
    
    // Copy authentication checkboxes
    collection->requests[index].auth_api_key_enabled = request->auth_api_key_enabled;
    collection->requests[index].auth_bearer_enabled = request->auth_bearer_enabled;
    collection->requests[index].auth_basic_enabled = request->auth_basic_enabled;
    collection->requests[index].auth_oauth_enabled = request->auth_oauth_enabled;
    
    printf("DEBUG: collection_add_request - copied auth data: type=%d, api_key_name='%s', bearer_token='%s', api_key_enabled=%s\n",
           collection->requests[index].selected_auth_type,
           collection->requests[index].auth_api_key_name,
           collection->requests[index].auth_bearer_token,
           collection->requests[index].auth_api_key_enabled ? "true" : "false");

    collection->request_names[index] = malloc(name_len + 1);
    if (!collection->request_names[index]) {
        handle_collections_out_of_memory("request name allocation");

        request_cleanup(&collection->requests[index]);
        return -1;
    }
    memcpy(collection->request_names[index], name, name_len);
    collection->request_names[index][name_len] = '\0';

    collection->request_count++;
    collection_update_modified_time(collection);

    return index;
}

int collection_remove_request(Collection* collection, int request_index) {
    if (!collection || request_index < 0 || request_index >= collection->request_count) {
        return -1;
    }

    request_cleanup(&collection->requests[request_index]);
    if (collection->request_names[request_index]) {
        free(collection->request_names[request_index]);
    }

    for (int i = request_index; i < collection->request_count - 1; i++) {
        collection->requests[i] = collection->requests[i + 1];
        collection->request_names[i] = collection->request_names[i + 1];
    }

    collection->request_count--;
    collection->request_names[collection->request_count] = NULL;
    collection_update_modified_time(collection);

    return 0;
}

int collection_duplicate_request(Collection* collection, int request_index) {
    if (!collection || request_index < 0 || request_index >= collection->request_count) {
        return -1;
    }

    const char* original_name = collection->request_names[request_index];
    char new_name[256];
    snprintf(new_name, sizeof(new_name), "%s (Copy)", original_name);

    return collection_add_request(collection, &collection->requests[request_index], new_name);
}

int collection_rename_request(Collection* collection, int request_index, const char* new_name) {
    if (!collection || request_index < 0 || request_index >= collection->request_count || !new_name) {
        return -1;
    }

    if (collection->request_names[request_index]) {
        free(collection->request_names[request_index]);
    }

    collection->request_names[request_index] = malloc(strlen(new_name) + 1);
    if (!collection->request_names[request_index]) {
        handle_collections_out_of_memory("request name reallocation");
        return -1;
    }

    strncpy(collection->request_names[request_index], new_name, strlen(new_name));
    collection->request_names[request_index][strlen(new_name)] = '\0';
    collection_update_modified_time(collection);

    return 0;
}

Request* collection_get_request(Collection* collection, int request_index) {
    if (!collection || request_index < 0 || request_index >= collection->request_count) {
        return NULL;
    }

    return &collection->requests[request_index];
}

const char* collection_get_request_name(Collection* collection, int request_index) {
    if (!collection || request_index < 0 || request_index >= collection->request_count) {
        return NULL;
    }

    return collection->request_names[request_index];
}

int collection_set_name(Collection* collection, const char* name) {
    if (!collection || !name || !collection_validate_name(name)) {
        return -1;
    }

    strncpy(collection->name, name, sizeof(collection->name) - 1);
    collection->name[sizeof(collection->name) - 1] = '\0';
    collection_update_modified_time(collection);

    return 0;
}

int collection_set_description(Collection* collection, const char* description) {
    if (!collection) {
        return -1;
    }

    const char* desc = description ? description : "";
    if (!collection_validate_description(desc)) {
        return -1;
    }

    strncpy(collection->description, desc, sizeof(collection->description) - 1);
    collection->description[sizeof(collection->description) - 1] = '\0';
    collection_update_modified_time(collection);

    return 0;
}

void collection_update_modified_time(Collection* collection) {
    if (collection) {
        collection->modified_at = time(NULL);
    }
}

CollectionManager* collection_manager_create(void) {
    CollectionManager* manager = malloc(sizeof(CollectionManager));
    if (!manager) {
        handle_collections_out_of_memory("collection manager creation");
        return NULL;
    }

    collection_manager_init(manager);
    return manager;
}

void collection_manager_destroy(CollectionManager* manager) {
    if (!manager) {
        return;
    }

    collection_manager_cleanup(manager);
    free(manager);
}

void collection_manager_init(CollectionManager* manager) {
    if (!manager) {
        return;
    }

    manager->collections = malloc(DEFAULT_COLLECTION_CAPACITY * sizeof(Collection));
    if (!manager->collections) {
        handle_collections_out_of_memory("collection manager initialization");
        manager->count = 0;
        manager->capacity = 0;
        manager->active_collection_index = -1;
        manager->active_request_index = -1;
        return;
    }

    manager->count = 0;
    manager->capacity = DEFAULT_COLLECTION_CAPACITY;
    manager->active_collection_index = -1;
    manager->active_request_index = -1;
}

void collection_manager_cleanup(CollectionManager* manager) {
    if (!manager) {
        return;
    }

    for (int i = 0; i < manager->count; i++) {
        collection_cleanup(&manager->collections[i]);
    }

    free(manager->collections);
    manager->collections = NULL;
    manager->count = 0;
    manager->capacity = 0;
    manager->active_collection_index = -1;
    manager->active_request_index = -1;
}

int collection_manager_add_collection(CollectionManager* manager, Collection* collection) {
    if (!manager || !collection) {
        return -1;
    }

    if (manager->count >= manager->capacity) {
        if (resize_array((void**)&manager->collections, &manager->capacity, sizeof(Collection)) != 0) {
            return -1;
        }
    }

    int index = manager->count;

    manager->collections[index].requests = malloc(collection->request_capacity * sizeof(Request));
    manager->collections[index].request_names = malloc(collection->request_capacity * sizeof(char*));
    manager->collections[index].request_count = 0;
    manager->collections[index].request_capacity = collection->request_capacity;

    strncpy(manager->collections[index].id, collection->id, sizeof(manager->collections[index].id) - 1);
    manager->collections[index].id[sizeof(manager->collections[index].id) - 1] = '\0';

    strncpy(manager->collections[index].name, collection->name, sizeof(manager->collections[index].name) - 1);
    manager->collections[index].name[sizeof(manager->collections[index].name) - 1] = '\0';

    strncpy(manager->collections[index].description, collection->description, sizeof(manager->collections[index].description) - 1);
    manager->collections[index].description[sizeof(manager->collections[index].description) - 1] = '\0';

    manager->collections[index].created_at = collection->created_at;
    manager->collections[index].modified_at = collection->modified_at;

    cookie_jar_init(&manager->collections[index].cookie_jar);

    for (int i = 0; i < collection->cookie_jar.count; i++) {
        StoredCookie* src_cookie = &collection->cookie_jar.cookies[i];
        cookie_jar_add_cookie(&manager->collections[index].cookie_jar,
                             src_cookie->name, src_cookie->value,
                             src_cookie->domain, src_cookie->path,
                             src_cookie->expires, src_cookie->max_age,
                             src_cookie->secure, src_cookie->http_only,
                             src_cookie->same_site_strict, src_cookie->same_site_lax);
    }

    printf("DEBUG: Copied %d cookies to collection manager (source had %d cookies)\n",
           manager->collections[index].cookie_jar.count, collection->cookie_jar.count);

    for (int i = 0; i < collection->request_capacity; i++) {
        manager->collections[index].request_names[i] = NULL;
    }

    for (int i = 0; i < collection->request_count; i++) {
        collection_add_request(&manager->collections[index],
                              &collection->requests[i],
                              collection->request_names[i]);
    }

    manager->count++;

    if (manager->active_collection_index == -1) {
        manager->active_collection_index = index;
        manager->active_request_index = collection->request_count > 0 ? 0 : -1;
    }

    return index;
}

int collection_manager_remove_collection(CollectionManager* manager, int collection_index) {
    if (!manager || collection_index < 0 || collection_index >= manager->count) {
        return -1;
    }

    collection_cleanup(&manager->collections[collection_index]);

    for (int i = collection_index; i < manager->count - 1; i++) {
        manager->collections[i] = manager->collections[i + 1];
    }

    manager->count--;

    if (manager->active_collection_index == collection_index) {
        manager->active_collection_index = manager->count > 0 ? 0 : -1;
        manager->active_request_index = -1;

        if (manager->active_collection_index >= 0) {
            Collection* active_collection = &manager->collections[manager->active_collection_index];
            if (active_collection->request_count > 0) {
                manager->active_request_index = 0;
            }
        }
    } else if (manager->active_collection_index > collection_index) {
        manager->active_collection_index--;
    }

    return 0;
}

int collection_manager_duplicate_collection(CollectionManager* manager, int collection_index) {
    if (!manager || collection_index < 0 || collection_index >= manager->count) {
        return -1;
    }

    Collection* original = &manager->collections[collection_index];

    char new_name[256];
    snprintf(new_name, sizeof(new_name), "%s (Copy)", original->name);

    Collection* new_collection = collection_create(new_name, original->description);
    if (!new_collection) {
        return -1;
    }

    for (int i = 0; i < original->request_count; i++) {
        collection_add_request(new_collection, &original->requests[i], original->request_names[i]);
    }

    int result = collection_manager_add_collection(manager, new_collection);
    collection_destroy(new_collection); 

    return result;
}

Collection* collection_manager_get_collection(CollectionManager* manager, int collection_index) {
    if (!manager || collection_index < 0 || collection_index >= manager->count) {
        return NULL;
    }

    return &manager->collections[collection_index];
}

Collection* collection_manager_get_active_collection(CollectionManager* manager) {
    if (!manager || manager->active_collection_index < 0 || manager->active_collection_index >= manager->count) {
        return NULL;
    }

    return &manager->collections[manager->active_collection_index];
}

Request* collection_manager_get_active_request(CollectionManager* manager) {
    Collection* active_collection = collection_manager_get_active_collection(manager);
    if (!active_collection || manager->active_request_index < 0 || 
        manager->active_request_index >= active_collection->request_count) {
        return NULL;
    }

    return &active_collection->requests[manager->active_request_index];
}

int collection_manager_set_active_collection(CollectionManager* manager, int collection_index) {
    if (!manager || collection_index < -1 || collection_index >= manager->count) {
        return -1;
    }

    manager->active_collection_index = collection_index;

    if (collection_index >= 0) {
        Collection* collection = &manager->collections[collection_index];
        manager->active_request_index = collection->request_count > 0 ? 0 : -1;
    } else {
        manager->active_request_index = -1;
    }

    return 0;
}

int collection_manager_set_active_request(CollectionManager* manager, int request_index) {
    if (!manager) {
        return -1;
    }

    Collection* active_collection = collection_manager_get_active_collection(manager);
    if (!active_collection) {
        return -1;
    }

    if (request_index < -1 || request_index >= active_collection->request_count) {
        return -1;
    }

    manager->active_request_index = request_index;
    return 0;
}

int collection_manager_find_collection_by_name(CollectionManager* manager, const char* name) {
    if (!manager || !name) {
        return -1;
    }

    for (int i = 0; i < manager->count; i++) {
        if (strcmp(manager->collections[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

int collection_manager_get_total_requests(CollectionManager* manager) {
    if (!manager) {
        return 0;
    }

    int total = 0;
    for (int i = 0; i < manager->count; i++) {
        total += manager->collections[i].request_count;
    }

    return total;
}

bool collection_manager_has_collections(CollectionManager* manager) {
    return manager && manager->count > 0;
}

bool collection_validate_name(const char* name) {
    if (!name || strlen(name) == 0 || strlen(name) >= 256) {
        return false;
    }

    for (const char* p = name; *p; p++) {
        if (*p == '\n' || *p == '\r' || *p == '\t') {
            return false;
        }
    }

    return true;
}

bool collection_validate_description(const char* description) {
    if (!description) {
        return true; 
    }

    if (strlen(description) >= 512) {
        return false;
    }

    return true;
}

bool collection_is_valid(const Collection* collection) {
    if (!collection) {
        return false;
    }

    if (!collection_validate_name(collection->name)) {
        return false;
    }

    if (!collection_validate_description(collection->description)) {
        return false;
    }

    if (collection->request_count < 0 || collection->request_capacity < collection->request_count) {
        return false;
    }

    if (collection->request_count > 0 && (!collection->requests || !collection->request_names)) {
        return false;
    }

    return true;
}

void collections_set_out_of_memory_handler(void (*handler)(const char* operation)) {
    g_collections_out_of_memory_handler = handler;
}

CookieJar* cookie_jar_create(void) {
    CookieJar* jar = malloc(sizeof(CookieJar));
    if (!jar) {
        handle_collections_out_of_memory("cookie jar creation");
        return NULL;
    }

    cookie_jar_init(jar);
    return jar;
}

void cookie_jar_destroy(CookieJar* jar) {
    if (!jar) {
        return;
    }

    cookie_jar_cleanup(jar);
    free(jar);
}

void cookie_jar_init(CookieJar* jar) {
    if (!jar) {
        return;
    }

    jar->cookies = malloc(DEFAULT_COOKIE_CAPACITY * sizeof(StoredCookie));
    if (!jar->cookies) {
        handle_collections_out_of_memory("cookie jar initialization");
        jar->count = 0;
        jar->capacity = 0;
        return;
    }

    jar->count = 0;
    jar->capacity = DEFAULT_COOKIE_CAPACITY;

    for (int i = 0; i < DEFAULT_COOKIE_CAPACITY; i++) {
        memset(&jar->cookies[i], 0, sizeof(StoredCookie));
    }
}

void cookie_jar_cleanup(CookieJar* jar) {
    if (!jar) {
        return;
    }

    for (int i = 0; i < jar->count; i++) {
        stored_cookie_cleanup(&jar->cookies[i]);
    }

    free(jar->cookies);
    jar->cookies = NULL;
    jar->count = 0;
    jar->capacity = 0;
}

void stored_cookie_cleanup(StoredCookie* cookie) {
    if (!cookie) {
        return;
    }

    memset(cookie->name, 0, sizeof(cookie->name));
    memset(cookie->value, 0, sizeof(cookie->value));
    memset(cookie->domain, 0, sizeof(cookie->domain));
    memset(cookie->path, 0, sizeof(cookie->path));

    cookie->expires = 0;
    cookie->max_age = -1;
    cookie->secure = false;
    cookie->http_only = false;
    cookie->same_site_strict = false;
    cookie->same_site_lax = false;
    cookie->created_at = 0;
}

int cookie_jar_add_cookie(CookieJar* jar, const char* name, const char* value,
                         const char* domain, const char* path, time_t expires,
                         int max_age, bool secure, bool http_only,
                         bool same_site_strict, bool same_site_lax) {
    if (!jar || !name || !value) {
        return -1;
    }

    if (strlen(name) >= sizeof(((StoredCookie*)0)->name) ||
        strlen(value) >= sizeof(((StoredCookie*)0)->value)) {
        return -1;
    }

    if (domain && strlen(domain) >= sizeof(((StoredCookie*)0)->domain)) {
        return -1;
    }

    if (path && strlen(path) >= sizeof(((StoredCookie*)0)->path)) {
        return -1;
    }

    int existing_index = cookie_jar_find_cookie(jar, name, domain, path);
    if (existing_index >= 0) {

        StoredCookie* cookie = &jar->cookies[existing_index];

        strncpy(cookie->value, value, sizeof(cookie->value) - 1);
        cookie->value[sizeof(cookie->value) - 1] = '\0';

        cookie->expires = expires;
        cookie->max_age = max_age;
        cookie->secure = secure;
        cookie->http_only = http_only;
        cookie->same_site_strict = same_site_strict;
        cookie->same_site_lax = same_site_lax;
        cookie->created_at = time(NULL);

        return existing_index;
    }

    if (jar->count >= jar->capacity) {
        if (resize_array((void**)&jar->cookies, &jar->capacity, sizeof(StoredCookie)) != 0) {
            return -1;
        }

        for (int i = jar->count; i < jar->capacity; i++) {
            memset(&jar->cookies[i], 0, sizeof(StoredCookie));
        }
    }

    int index = jar->count;
    StoredCookie* cookie = &jar->cookies[index];

    strncpy(cookie->name, name, sizeof(cookie->name) - 1);
    cookie->name[sizeof(cookie->name) - 1] = '\0';

    strncpy(cookie->value, value, sizeof(cookie->value) - 1);
    cookie->value[sizeof(cookie->value) - 1] = '\0';

    if (domain) {
        strncpy(cookie->domain, domain, sizeof(cookie->domain) - 1);
        cookie->domain[sizeof(cookie->domain) - 1] = '\0';
    } else {
        cookie->domain[0] = '\0';
    }

    if (path) {
        strncpy(cookie->path, path, sizeof(cookie->path) - 1);
        cookie->path[sizeof(cookie->path) - 1] = '\0';
    } else {
        strncpy(cookie->path, "/", sizeof(cookie->path) - 1); 
        cookie->path[sizeof(cookie->path) - 1] = '\0';
    }

    cookie->expires = expires;
    cookie->max_age = max_age;
    cookie->secure = secure;
    cookie->http_only = http_only;
    cookie->same_site_strict = same_site_strict;
    cookie->same_site_lax = same_site_lax;
    cookie->created_at = time(NULL);

    jar->count++;
    return index;
}

int cookie_jar_remove_cookie(CookieJar* jar, int cookie_index) {
    if (!jar || cookie_index < 0 || cookie_index >= jar->count) {
        return -1;
    }

    stored_cookie_cleanup(&jar->cookies[cookie_index]);

    for (int i = cookie_index; i < jar->count - 1; i++) {
        jar->cookies[i] = jar->cookies[i + 1];
    }

    jar->count--;

    memset(&jar->cookies[jar->count], 0, sizeof(StoredCookie));

    return 0;
}

int cookie_jar_find_cookie(CookieJar* jar, const char* name, const char* domain, const char* path) {
    if (!jar || !name) {
        return -1;
    }

    const char* safe_domain = domain ? domain : "";
    const char* safe_path = path ? path : "/";

    for (int i = 0; i < jar->count; i++) {
        StoredCookie* cookie = &jar->cookies[i];

        if (strcmp(cookie->name, name) == 0 &&
            strcmp(cookie->domain, safe_domain) == 0 &&
            strcmp(cookie->path, safe_path) == 0) {
            return i;
        }
    }

    return -1;
}

StoredCookie* cookie_jar_get_cookie(CookieJar* jar, int cookie_index) {
    if (!jar || cookie_index < 0 || cookie_index >= jar->count) {
        return NULL;
    }

    return &jar->cookies[cookie_index];
}

bool cookie_jar_is_cookie_expired(const StoredCookie* cookie) {
    if (!cookie) {
        return true;
    }

    time_t now = time(NULL);

    if (cookie->max_age >= 0) {
        time_t expiry_time = cookie->created_at + cookie->max_age;
        return now > expiry_time;
    }

    if (cookie->expires > 0) {
        return now > cookie->expires;
    }

    return false;
}

int cookie_jar_cleanup_expired(CookieJar* jar) {
    if (!jar) {
        return 0;
    }

    int removed_count = 0;

    for (int i = jar->count - 1; i >= 0; i--) {
        if (cookie_jar_is_cookie_expired(&jar->cookies[i])) {
            cookie_jar_remove_cookie(jar, i);
            removed_count++;
        }
    }

    return removed_count;
}

bool cookie_jar_matches_request(const StoredCookie* cookie, const char* url, bool is_secure) {
    if (!cookie || !url) {
        return false;
    }

    if (cookie_jar_is_cookie_expired(cookie)) {
        return false;
    }

    if (cookie->secure && !is_secure) {

        char domain[256] = {0};
        const char* protocol_end = strstr(url, "://");
        if (protocol_end) {
            const char* domain_start = protocol_end + 3;
            const char* path_start = strchr(domain_start, '/');

            if (path_start) {
                size_t domain_len = path_start - domain_start;
                if (domain_len >= sizeof(domain)) {
                    domain_len = sizeof(domain) - 1;
                }
                memcpy(domain, domain_start, domain_len);
                domain[domain_len] = '\0';
            } else {
                strncpy(domain, domain_start, sizeof(domain) - 1);
                domain[sizeof(domain) - 1] = '\0';
            }

            char* port_pos = strchr(domain, ':');
            if (port_pos) {
                *port_pos = '\0';
            }

            if (strcmp(domain, "localhost") != 0 && strcmp(domain, "127.0.0.1") != 0) {
                return false;
            }
        } else {
            return false;
        }
    }

    char domain[256] = {0};
    char path[512] = {0};

    const char* protocol_end = strstr(url, "://");
    if (!protocol_end) {
        return false;
    }

    const char* domain_start = protocol_end + 3;
    const char* path_start = strchr(domain_start, '/');

    if (path_start) {

        size_t domain_len = path_start - domain_start;
        if (domain_len >= sizeof(domain)) {
            domain_len = sizeof(domain) - 1;
        }
        memcpy(domain, domain_start, domain_len);
        domain[domain_len] = '\0';

        strncpy(path, path_start, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    } else {

        strncpy(domain, domain_start, sizeof(domain) - 1);
        domain[sizeof(domain) - 1] = '\0';
        strncpy(path, "/", sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    }

    char* port_pos = strchr(domain, ':');
    if (port_pos) {
        *port_pos = '\0';
    }

    if (strlen(cookie->domain) > 0) {

        if (cookie->domain[0] == '.') {

            const char* cookie_domain = cookie->domain + 1; 
            size_t cookie_domain_len = strlen(cookie_domain);
            size_t request_domain_len = strlen(domain);

            if (request_domain_len < cookie_domain_len) {
                return false;
            }

            const char* suffix = domain + request_domain_len - cookie_domain_len;
            if (strcmp(suffix, cookie_domain) != 0) {
                return false;
            }

            /* make sure it's a proper subdomain match */
            if (request_domain_len > cookie_domain_len &&
                domain[request_domain_len - cookie_domain_len - 1] != '.') {
                return false;
            }
        } else {
            /* exact domain match required */
            if (strcmp(cookie->domain, domain) != 0) {
                return false;
            }
        }
    }

    /* check path match */
    if (strlen(cookie->path) > 0) {
        /* cookie has specific path */
        size_t cookie_path_len = strlen(cookie->path);

        /* request path must start with cookie path */
        if (strncmp(path, cookie->path, cookie_path_len) != 0) {
            return false;
        }

        /* if cookie path doesn't end with '/', the next character in request path must be '/' or end of string */
        if (cookie->path[cookie_path_len - 1] != '/' &&
            path[cookie_path_len] != '\0' &&
            path[cookie_path_len] != '/') {
            return false;
        }
    }

    return true;
}

/* gets all cookies that match the given request */
int cookie_jar_get_cookies_for_request(CookieJar* jar, const char* url, bool is_secure,
                                      StoredCookie** matching_cookies, int max_cookies) {
    if (!jar || !url || !matching_cookies) {
        return 0;
    }

    int match_count = 0;

    for (int i = 0; i < jar->count && match_count < max_cookies; i++) {
        if (cookie_jar_matches_request(&jar->cookies[i], url, is_secure)) {
            matching_cookies[match_count] = &jar->cookies[i];
            match_count++;
        }
    }

    return match_count;
}

/* builds a cookie header string for the given request */
char* cookie_jar_build_cookie_header(CookieJar* jar, const char* url, bool is_secure) {
    if (!jar || !url) {
        return NULL;
    }

    /* get matching cookies */
    StoredCookie* matching_cookies[256]; /* max 256 cookies per request */
    int match_count = cookie_jar_get_cookies_for_request(jar, url, is_secure, matching_cookies, 256);

    if (match_count == 0) {
        return NULL;
    }

    /* calculate required buffer size */
    size_t total_size = 0;
    for (int i = 0; i < match_count; i++) {
        total_size += strlen(matching_cookies[i]->name);
        total_size += 1; 
        total_size += strlen(matching_cookies[i]->value);
        if (i < match_count - 1) {
            total_size += 2; 
        }
    }
    total_size += 1; 

    /* allocate buffer */
    char* cookie_header = malloc(total_size);
    if (!cookie_header) {
        handle_collections_out_of_memory("cookie header building");
        return NULL;
    }

    /* build cookie header */
    cookie_header[0] = '\0';
    for (int i = 0; i < match_count; i++) {
        size_t current_len = strlen(cookie_header);
        size_t remaining = total_size - current_len - 1;

        size_t name_len = strlen(matching_cookies[i]->name);
        if (name_len < remaining) {
            strncat(cookie_header, matching_cookies[i]->name, remaining);
            current_len += name_len;
            remaining -= name_len;
        }

        if (remaining > 0) {
            strncat(cookie_header, "=", remaining);
            current_len += 1;
            remaining -= 1;
        }

        size_t value_len = strlen(matching_cookies[i]->value);
        if (value_len < remaining) {
            strncat(cookie_header, matching_cookies[i]->value, remaining);
            current_len += value_len;
            remaining -= value_len;
        }

        if (i < match_count - 1 && remaining > 2) {
            strncat(cookie_header, "; ", remaining);
        }
    }

    return cookie_header;
}

/* removes all cookies from the jar */
void cookie_jar_clear_all(CookieJar* jar) {
    if (!jar) {
        return;
    }

    /* clean up all cookies */
    for (int i = 0; i < jar->count; i++) {
        stored_cookie_cleanup(&jar->cookies[i]);
    }

    jar->count = 0;
}

/* parses a set-cookie header and adds the cookie to the jar */
int cookie_jar_parse_set_cookie(CookieJar* jar, const char* set_cookie_header, const char* request_url) {
    if (!jar || !set_cookie_header || !request_url) {
        return -1;
    }

    /* parse the set-cookie header */
    char name[256] = {0};
    char value[1024] = {0};
    char domain[256] = {0};
    char path[512] = {0};
    time_t expires = 0;
    int max_age = -1;
    bool secure = false;
    bool http_only = false;
    bool same_site_strict = false;
    bool same_site_lax = false;

    /* create a working copy of the header */
    char* header_copy = malloc(strlen(set_cookie_header) + 1);
    if (!header_copy) {
        handle_collections_out_of_memory("set-cookie parsing");
        return -1;
    }
    strncpy(header_copy, set_cookie_header, strlen(set_cookie_header));
    header_copy[strlen(set_cookie_header)] = '\0';

    /* parse name=value pair (first part) */
    char* token = strtok(header_copy, ";");
    if (!token) {
        free(header_copy);
        return -1;
    }

    /* extract name and value */
    char* equals_pos = strchr(token, '=');
    if (!equals_pos) {
        free(header_copy);
        return -1;
    }

    /* copy name (trim whitespace) */
    size_t name_len = equals_pos - token;
    while (name_len > 0 && (token[name_len - 1] == ' ' || token[name_len - 1] == '\t')) {
        name_len--;
    }
    if (name_len >= sizeof(name)) {
        name_len = sizeof(name) - 1;
    }
    memcpy(name, token, name_len);
    name[name_len] = '\0';

    /* copy value (trim whitespace) */
    const char* value_start = equals_pos + 1;
    while (*value_start == ' ' || *value_start == '\t') {
        value_start++;
    }
    strncpy(value, value_start, sizeof(value) - 1);
    value[sizeof(value) - 1] = '\0';

    /* remove trailing whitespace from value */
    size_t value_len = strlen(value);
    while (value_len > 0 && (value[value_len - 1] == ' ' || value[value_len - 1] == '\t')) {
        value[--value_len] = '\0';
    }

    /* parse attributes */
    while ((token = strtok(NULL, ";")) != NULL) {

        while (*token == ' ' || *token == '\t') {
            token++;
        }

        if (strncasecmp(token, "domain=", 7) == 0) {
            strncpy(domain, token + 7, sizeof(domain) - 1);
            domain[sizeof(domain) - 1] = '\0';
        } else if (strncasecmp(token, "path=", 5) == 0) {
            strncpy(path, token + 5, sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
        } else if (strncasecmp(token, "expires=", 8) == 0) {

            expires = time(NULL) + 3600; 
        } else if (strncasecmp(token, "max-age=", 8) == 0) {
            max_age = atoi(token + 8);
        } else if (strcasecmp(token, "secure") == 0) {
            secure = true;
        } else if (strcasecmp(token, "httponly") == 0) {
            http_only = true;
        } else if (strncasecmp(token, "samesite=", 9) == 0) {
            const char* samesite_value = token + 9;
            if (strcasecmp(samesite_value, "strict") == 0) {
                same_site_strict = true;
                same_site_lax = false;
            } else if (strcasecmp(samesite_value, "lax") == 0) {
                same_site_strict = false;
                same_site_lax = true;
            } else {
                same_site_strict = false;
                same_site_lax = false;
            }
        }
    }

    /* if no domain specified, extract from request url */
    if (strlen(domain) == 0) {
        const char* protocol_end = strstr(request_url, "://");
        if (protocol_end) {
            const char* domain_start = protocol_end + 3;
            const char* path_start = strchr(domain_start, '/');

            size_t domain_len;
            if (path_start) {
                domain_len = path_start - domain_start;
            } else {
                domain_len = strlen(domain_start);
            }

            if (domain_len >= sizeof(domain)) {
                domain_len = sizeof(domain) - 1;
            }
            memcpy(domain, domain_start, domain_len);
            domain[domain_len] = '\0';

            char* port_pos = strchr(domain, ':');
            if (port_pos) {
                *port_pos = '\0';
            }
        }
    }

    /* if no path specified, use default */
    if (strlen(path) == 0) {
        strncpy(path, "/", sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    }

    free(header_copy);

    /* add cookie to jar */
    return cookie_jar_add_cookie(jar, name, value, domain, path, expires, max_age,
                                secure, http_only, same_site_strict, same_site_lax);
}