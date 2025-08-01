/**
 * data persistence system for tinyrequest
 *
 * this file handles all the saving and loading of application data including
 * individual requests, collections, settings, and application state. it uses
 * json format for human-readable storage and provides comprehensive error
 * handling and data validation.
 *
 * the persistence system supports both individual request storage and the
 * newer collection-based system. it handles migration from legacy formats,
 * auto-save functionality, and maintains data integrity through validation
 * and backup mechanisms.
 *
 * all data is stored in platform-appropriate locations - appdata on windows
 * and .config directories on unix systems.
 */

#include "persistence.h"
#include "app_state.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* windows-specific includes for file operations */
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif

/* saves a single request to a json file */
int persistence_save_request(const Request* request, const char* name, const char* filename) {
    if (!request || !name || !filename) {
        return -1;
    }

    /* create json object for the request */
    cJSON* json = cJSON_CreateObject();
    if (!json) {
        return -1;
    }

    /* add request name */
    cJSON* json_name = cJSON_CreateString(name);
    if (!json_name) {
        cJSON_Delete(json);
        return -1;
    }
    cJSON_AddItemToObject(json, "name", json_name);

    /* add http method */
    cJSON* json_method = cJSON_CreateString(request->method);
    if (!json_method) {
        cJSON_Delete(json);
        return -1;
    }
    cJSON_AddItemToObject(json, "method", json_method);

    /* add url */
    cJSON* json_url = cJSON_CreateString(request->url);
    if (!json_url) {
        cJSON_Delete(json);
        return -1;
    }
    cJSON_AddItemToObject(json, "url", json_url);

    /* add headers array */
    cJSON* json_headers = cJSON_CreateArray();
    if (!json_headers) {
        cJSON_Delete(json);
        return -1;
    }

    for (int i = 0; i < request->headers.count; i++) {
        cJSON* header_obj = cJSON_CreateObject();
        if (!header_obj) {
            cJSON_Delete(json);
            return -1;
        }

        cJSON* header_name = cJSON_CreateString(request->headers.headers[i].name);
        cJSON* header_value = cJSON_CreateString(request->headers.headers[i].value);

        if (!header_name || !header_value) {
            cJSON_Delete(header_obj);
            cJSON_Delete(json);
            return -1;
        }

        cJSON_AddItemToObject(header_obj, "name", header_name);
        cJSON_AddItemToObject(header_obj, "value", header_value);
        cJSON_AddItemToArray(json_headers, header_obj);
    }
    cJSON_AddItemToObject(json, "headers", json_headers);

    /* add request body (can be empty) */
    const char* body_content = request->body ? request->body : "";
    cJSON* json_body = cJSON_CreateString(body_content);
    if (!json_body) {
        cJSON_Delete(json);
        return -1;
    }
    cJSON_AddItemToObject(json, "body", json_body);

    /* convert json to string */
    char* json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        return -1;
    }

    /* get full file path */
    char* full_path = persistence_get_config_path(filename);
    if (!full_path) {
        free(json_string);
        return -1;
    }

    /* create config directory if it doesn't exist */
    if (persistence_create_config_dir() != 0) {
        free(json_string);
        free(full_path);
        return -1;
    }

    /* write json to file */
    FILE* file = fopen(full_path, "w");
    if (!file) {
        free(json_string);
        free(full_path);
        return -1;
    }

    size_t json_len = strlen(json_string);
    size_t written = fwrite(json_string, 1, json_len, file);
    fclose(file);

    free(json_string);
    free(full_path);

    return (written == json_len) ? 0 : -1;
}

/* loads a single request from a json file */
int persistence_load_request(Request* request, const char* filename) {
    if (!request || !filename) {
        return -1;
    }

    /* get full file path */
    char* full_path = persistence_get_config_path(filename);
    if (!full_path) {
        return -1;
    }

    /* check if file exists */
    if (!persistence_file_exists(full_path)) {
        free(full_path);
        return -1;
    }

    /* read file content */
    FILE* file = fopen(full_path, "r");
    if (!file) {
        free(full_path);
        return -1;
    }

    /* get file size */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        free(full_path);
        return -1;
    }

    /* read file content */
    char* json_string = (char*)malloc(file_size + 1);
    if (!json_string) {
        fclose(file);
        free(full_path);
        return -1;
    }

    size_t read_size = fread(json_string, 1, file_size, file);
    json_string[read_size] = '\0';
    fclose(file);
    free(full_path);

    if (read_size != (size_t)file_size) {
        free(json_string);
        return -1;
    }

    /* parse json */
    cJSON* json = cJSON_Parse(json_string);
    free(json_string);

    if (!json) {
        return -1;
    }

    /* clean up existing request data */
    request_cleanup(request);
    request_init(request);

    /* extract method */
    cJSON* json_method = cJSON_GetObjectItem(json, "method");
    if (json_method && cJSON_IsString(json_method)) {
        strncpy(request->method, json_method->valuestring, sizeof(request->method) - 1);
        request->method[sizeof(request->method) - 1] = '\0';
    }

    /* extract url */
    cJSON* json_url = cJSON_GetObjectItem(json, "url");
    if (json_url && cJSON_IsString(json_url)) {
        strncpy(request->url, json_url->valuestring, sizeof(request->url) - 1);
        request->url[sizeof(request->url) - 1] = '\0';
    }

    /* extract headers */
    cJSON* json_headers = cJSON_GetObjectItem(json, "headers");
    if (json_headers && cJSON_IsArray(json_headers)) {
        cJSON* header_item = NULL;
        cJSON_ArrayForEach(header_item, json_headers) {
            if (cJSON_IsObject(header_item)) {
                cJSON* header_name = cJSON_GetObjectItem(header_item, "name");
                cJSON* header_value = cJSON_GetObjectItem(header_item, "value");

                if (header_name && cJSON_IsString(header_name) &&
                    header_value && cJSON_IsString(header_value)) {
                    header_list_add(&request->headers, header_name->valuestring, header_value->valuestring);
                }
            }
        }
    }

    /* extract body */
    cJSON* json_body = cJSON_GetObjectItem(json, "body");
    if (json_body && cJSON_IsString(json_body)) {
        const char* body_content = json_body->valuestring;
        if (strlen(body_content) > 0) {
            request_set_body(request, body_content, strlen(body_content));
        }
    }

    cJSON_Delete(json);
    return 0;
}

/* creates the application configuration directory */
int persistence_create_config_dir(void) {
#ifdef _WIN32
    /* use appdata environment variable for better compatibility */
    char* appdata_path = getenv("LOCALAPPDATA");
    if (!appdata_path) {

        appdata_path = getenv("APPDATA");
        if (!appdata_path) {
            return -1;
        }
    }

    /* create tinyrequest directory path */
    char config_dir[MAX_PATH];
    snprintf(config_dir, sizeof(config_dir), "%s\\TinyRequest", appdata_path);

    /* create directory if it doesn't exist */
    DWORD attrs = GetFileAttributesA(config_dir);
    if (attrs == INVALID_FILE_ATTRIBUTES) {

        if (!CreateDirectoryA(config_dir, NULL)) {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS) {
                return -1;
            }
        }
    } else if (!(attrs & FILE_ATTRIBUTE_DIRECTORY)) {

        return -1;
    }

    return 0;
#else

    char* home = getenv("HOME");
    if (!home) {
        return -1;
    }

    char config_dir[1024];
    snprintf(config_dir, sizeof(config_dir), "%s/.config/tinyrequest", home);

    /* create directory with proper permissions */
    if (mkdir(config_dir, 0755) != 0) {

        struct stat st;
        if (stat(config_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
            return -1;
        }
    }

    return 0;
#endif
}

/* returns the full path to a config file */
char* persistence_get_config_path(const char* filename) {
    if (!filename) {
        return NULL;
    }

#ifdef _WIN32
    /* use appdata environment variable for better compatibility */
    char* appdata_path = getenv("LOCALAPPDATA");
    if (!appdata_path) {

        appdata_path = getenv("APPDATA");
        if (!appdata_path) {
            return NULL;
        }
    }

    /* create full path */
    char* full_path = (char*)malloc(MAX_PATH);
    if (!full_path) {
        return NULL;
    }

    snprintf(full_path, MAX_PATH, "%s\\TinyRequest\\%s", appdata_path, filename);
    return full_path;
#else
    /* unix/linux implementation */
    char* home = getenv("HOME");
    if (!home) {
        return NULL;
    }

    char* full_path = (char*)malloc(1024);
    if (!full_path) {
        return NULL;
    }

    snprintf(full_path, 1024, "%s/.config/tinyrequest/%s", home, filename);
    return full_path;
#endif
}

/* checks if a file exists at the given path */
int persistence_file_exists(const char* filepath) {
    if (!filepath) {
        printf("DEBUG: persistence_file_exists called with NULL filepath\n");
        return 0;
    }

    printf("DEBUG: Checking if file exists: %s\n", filepath);

#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(filepath);
    int exists = (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
    printf("DEBUG: File exists result: %s (attrs: %lu)\n", exists ? "YES" : "NO", attrs);
    return exists;
#else
    struct stat st;
    int result = (stat(filepath, &st) == 0 && S_ISREG(st.st_mode));
    printf("DEBUG: File exists result: %s\n", result ? "YES" : "NO");
    return result;
#endif
}

/* saves a collection to a json file using the new format */
int persistence_save_collection_new(const Collection* collection, const char* filepath) {
    if (!collection || !filepath) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    /* create json object for the collection */
    cJSON* json = cJSON_CreateObject();
    if (!json) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    /* add collection metadata */
    cJSON* json_id = cJSON_CreateString(collection->id);
    cJSON* json_name = cJSON_CreateString(collection->name);
    cJSON* json_description = cJSON_CreateString(collection->description);
    cJSON* json_created_at = cJSON_CreateNumber((double)collection->created_at);
    cJSON* json_modified_at = cJSON_CreateNumber((double)collection->modified_at);

    if (!json_id || !json_name || !json_description || !json_created_at || !json_modified_at) {
        cJSON_Delete(json);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    cJSON_AddItemToObject(json, "id", json_id);
    cJSON_AddItemToObject(json, "name", json_name);
    cJSON_AddItemToObject(json, "description", json_description);
    cJSON_AddItemToObject(json, "created_at", json_created_at);
    cJSON_AddItemToObject(json, "modified_at", json_modified_at);

    /* add requests array */
    cJSON* json_requests = cJSON_CreateArray();
    if (!json_requests) {
        cJSON_Delete(json);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    for (int i = 0; i < collection->request_count; i++) {
        cJSON* json_request = cJSON_CreateObject();
        if (!json_request) {
            cJSON_Delete(json);
            return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
        }

        const Request* request = &collection->requests[i];
        const char* name = collection->request_names[i] ? collection->request_names[i] : "Unnamed Request";

        cJSON* req_name = cJSON_CreateString(name);
        cJSON* req_method = cJSON_CreateString(request->method);
        cJSON* req_url = cJSON_CreateString(request->url);

        if (!req_name || !req_method || !req_url) {
            cJSON_Delete(json_request);
            cJSON_Delete(json);
            return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
        }

        cJSON_AddItemToObject(json_request, "name", req_name);
        cJSON_AddItemToObject(json_request, "method", req_method);
        cJSON_AddItemToObject(json_request, "url", req_url);

        cJSON* req_headers = cJSON_CreateArray();
        if (!req_headers) {
            cJSON_Delete(json_request);
            cJSON_Delete(json);
            return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
        }

        for (int j = 0; j < request->headers.count; j++) {
            cJSON* header_obj = cJSON_CreateObject();
            if (!header_obj) {
                cJSON_Delete(json_request);
                cJSON_Delete(json);
                return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
            }

            cJSON* header_name = cJSON_CreateString(request->headers.headers[j].name);
            cJSON* header_value = cJSON_CreateString(request->headers.headers[j].value);
            cJSON* header_enabled = cJSON_CreateBool(true); 

            if (!header_name || !header_value || !header_enabled) {
                cJSON_Delete(header_obj);
                cJSON_Delete(json_request);
                cJSON_Delete(json);
                return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
            }

            cJSON_AddItemToObject(header_obj, "name", header_name);
            cJSON_AddItemToObject(header_obj, "value", header_value);
            cJSON_AddItemToObject(header_obj, "enabled", header_enabled);
            cJSON_AddItemToArray(req_headers, header_obj);
        }
        cJSON_AddItemToObject(json_request, "headers", req_headers);

        cJSON* req_body = cJSON_CreateObject();
        if (!req_body) {
            cJSON_Delete(json_request);
            cJSON_Delete(json);
            return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
        }

        cJSON* body_type = cJSON_CreateString("raw");

        cJSON* body_content = cJSON_CreateString(request->body ? request->body : "");

        if (!body_type || !body_content) {
            cJSON_Delete(req_body);
            cJSON_Delete(json_request);
            cJSON_Delete(json);
            return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
        }

        cJSON_AddItemToObject(req_body, "type", body_type);
        cJSON_AddItemToObject(req_body, "content", body_content);
        cJSON_AddItemToObject(json_request, "body", req_body);

        cJSON* req_auth = cJSON_CreateObject();
        if (req_auth) {
            cJSON* auth_type = cJSON_CreateNumber(0); 
            cJSON_AddItemToObject(req_auth, "type", auth_type);
            cJSON_AddItemToObject(json_request, "auth", req_auth);
        }

        cJSON_AddItemToArray(json_requests, json_request);
    }
    cJSON_AddItemToObject(json, "requests", json_requests);

    /* convert json to string */
    char* json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    /* write json to file */
    FILE* file = fopen(filepath, "w");
    if (!file) {
        free(json_string);
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    size_t json_len = strlen(json_string);
    size_t written = fwrite(json_string, 1, json_len, file);
    fclose(file);
    free(json_string);

    return (written == json_len) ? PERSISTENCE_SUCCESS : PERSISTENCE_ERROR_DISK_FULL;
}

/* saves a collection with authentication data to a json file */
int persistence_save_collection_with_auth(const Collection* collection, const char* filepath, const void* app_state) {
    if (!collection || !filepath) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    const AppState* state = (const AppState*)app_state;

    /* create json object for the collection */
    cJSON* json = cJSON_CreateObject();
    if (!json) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    /* add collection metadata */
    cJSON* json_id = cJSON_CreateString(collection->id);
    cJSON* json_name = cJSON_CreateString(collection->name);
    cJSON* json_description = cJSON_CreateString(collection->description);
    cJSON* json_created_at = cJSON_CreateNumber((double)collection->created_at);
    cJSON* json_modified_at = cJSON_CreateNumber((double)collection->modified_at);

    if (!json_id || !json_name || !json_description || !json_created_at || !json_modified_at) {
        cJSON_Delete(json);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    cJSON_AddItemToObject(json, "id", json_id);
    cJSON_AddItemToObject(json, "name", json_name);
    cJSON_AddItemToObject(json, "description", json_description);
    cJSON_AddItemToObject(json, "created_at", json_created_at);
    cJSON_AddItemToObject(json, "modified_at", json_modified_at);

    /* add collection-level authentication data from appstate */
    if (state) {
        cJSON* collection_auth = cJSON_CreateObject();
        if (collection_auth) {
            cJSON* auth_type = cJSON_CreateNumber(state->selected_auth_type);
            cJSON_AddItemToObject(collection_auth, "type", auth_type);

            // Save authentication checkbox states
            cJSON* auth_api_key_enabled = cJSON_CreateBool(state->auth_api_key_enabled);
            cJSON* auth_bearer_enabled = cJSON_CreateBool(state->auth_bearer_enabled);
            cJSON* auth_basic_enabled = cJSON_CreateBool(state->auth_basic_enabled);
            cJSON* auth_oauth_enabled = cJSON_CreateBool(state->auth_oauth_enabled);
            
            if (auth_api_key_enabled && auth_bearer_enabled && auth_basic_enabled && auth_oauth_enabled) {
                cJSON_AddItemToObject(collection_auth, "api_key_enabled", auth_api_key_enabled);
                cJSON_AddItemToObject(collection_auth, "bearer_enabled", auth_bearer_enabled);
                cJSON_AddItemToObject(collection_auth, "basic_enabled", auth_basic_enabled);
                cJSON_AddItemToObject(collection_auth, "oauth_enabled", auth_oauth_enabled);
            }

            if (state->selected_auth_type == 1) {
                cJSON* api_key_name = cJSON_CreateString(state->auth_api_key_name);
                cJSON* api_key_value = cJSON_CreateString(state->auth_api_key_value);

                printf("DEBUG: Saving API key location: %d\n", state->auth_api_key_location);
                cJSON* api_key_location = cJSON_CreateNumber((double)state->auth_api_key_location);
                if (api_key_name && api_key_value && api_key_location) {
                    cJSON_AddItemToObject(collection_auth, "api_key_name", api_key_name);
                    cJSON_AddItemToObject(collection_auth, "api_key_value", api_key_value);
                    cJSON_AddItemToObject(collection_auth, "api_key_location", api_key_location);
                }
            } else if (state->selected_auth_type == 2) {
                cJSON* bearer_token = cJSON_CreateString(state->auth_bearer_token);
                if (bearer_token) {
                    cJSON_AddItemToObject(collection_auth, "bearer_token", bearer_token);
                }
            } else if (state->selected_auth_type == 3) {
                cJSON* basic_username = cJSON_CreateString(state->auth_basic_username);
                cJSON* basic_password = cJSON_CreateString(state->auth_basic_password);
                if (basic_username && basic_password) {
                    cJSON_AddItemToObject(collection_auth, "basic_username", basic_username);
                    cJSON_AddItemToObject(collection_auth, "basic_password", basic_password);
                }
            } else if (state->selected_auth_type == 4) {
                cJSON* oauth_token = cJSON_CreateString(state->auth_oauth_token);
                if (oauth_token) {
                    cJSON_AddItemToObject(collection_auth, "oauth_token", oauth_token);
                }
            }

            cJSON_AddItemToObject(json, "auth", collection_auth);
        }

        printf("DEBUG: Saving cookies to persistence - collection has %d cookies\n", collection->cookie_jar.count);

        if (collection->cookie_jar.count > 0) {
            cJSON* collection_cookies = cJSON_CreateArray();
            if (collection_cookies) {
                printf("DEBUG: Created cookies JSON array, saving %d cookies\n", collection->cookie_jar.count);
                for (int i = 0; i < collection->cookie_jar.count; i++) {
                    const StoredCookie* cookie = &collection->cookie_jar.cookies[i];
                    cJSON* cookie_obj = cJSON_CreateObject();
                    if (cookie_obj) {
                        cJSON* cookie_name = cJSON_CreateString(cookie->name);
                        cJSON* cookie_value = cJSON_CreateString(cookie->value);
                        cJSON* cookie_domain = cJSON_CreateString(cookie->domain);
                        cJSON* cookie_path = cJSON_CreateString(cookie->path);
                        cJSON* cookie_expires = cJSON_CreateNumber((double)cookie->expires);
                        cJSON* cookie_max_age = cJSON_CreateNumber((double)cookie->max_age);
                        cJSON* cookie_secure = cJSON_CreateBool(cookie->secure);
                        cJSON* cookie_http_only = cJSON_CreateBool(cookie->http_only);
                        cJSON* cookie_same_site_strict = cJSON_CreateBool(cookie->same_site_strict);
                        cJSON* cookie_same_site_lax = cJSON_CreateBool(cookie->same_site_lax);
                        cJSON* cookie_created_at = cJSON_CreateNumber((double)cookie->created_at);

                        if (cookie_name && cookie_value && cookie_domain && cookie_path &&
                            cookie_expires && cookie_max_age && cookie_secure && cookie_http_only &&
                            cookie_same_site_strict && cookie_same_site_lax && cookie_created_at) {
                            cJSON_AddItemToObject(cookie_obj, "name", cookie_name);
                            cJSON_AddItemToObject(cookie_obj, "value", cookie_value);
                            cJSON_AddItemToObject(cookie_obj, "domain", cookie_domain);
                            cJSON_AddItemToObject(cookie_obj, "path", cookie_path);
                            cJSON_AddItemToObject(cookie_obj, "expires", cookie_expires);
                            cJSON_AddItemToObject(cookie_obj, "max_age", cookie_max_age);
                            cJSON_AddItemToObject(cookie_obj, "secure", cookie_secure);
                            cJSON_AddItemToObject(cookie_obj, "http_only", cookie_http_only);
                            cJSON_AddItemToObject(cookie_obj, "same_site_strict", cookie_same_site_strict);
                            cJSON_AddItemToObject(cookie_obj, "same_site_lax", cookie_same_site_lax);
                            cJSON_AddItemToObject(cookie_obj, "created_at", cookie_created_at);
                            cJSON_AddItemToArray(collection_cookies, cookie_obj);
                        } else {
                            cJSON_Delete(cookie_obj);
                        }
                    }
                }
                cJSON_AddItemToObject(json, "cookies", collection_cookies);
            }
        }
    }

    /* add requests array */
    cJSON* json_requests = cJSON_CreateArray();
    if (!json_requests) {
        cJSON_Delete(json);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    for (int i = 0; i < collection->request_count; i++) {
        cJSON* json_request = cJSON_CreateObject();
        if (!json_request) {
            cJSON_Delete(json);
            return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
        }

        const Request* request = &collection->requests[i];
        const char* name = collection->request_names[i] ? collection->request_names[i] : "Unnamed Request";

        cJSON* req_name = cJSON_CreateString(name);
        cJSON* req_method = cJSON_CreateString(request->method);
        cJSON* req_url = cJSON_CreateString(request->url);

        if (!req_name || !req_method || !req_url) {
            cJSON_Delete(json_request);
            cJSON_Delete(json);
            return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
        }

        cJSON_AddItemToObject(json_request, "name", req_name);
        cJSON_AddItemToObject(json_request, "method", req_method);
        cJSON_AddItemToObject(json_request, "url", req_url);

        cJSON* req_headers = cJSON_CreateArray();
        if (!req_headers) {
            cJSON_Delete(json_request);
            cJSON_Delete(json);
            return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
        }

        for (int j = 0; j < request->headers.count; j++) {
            cJSON* header_obj = cJSON_CreateObject();
            if (!header_obj) {
                cJSON_Delete(json_request);
                cJSON_Delete(json);
                return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
            }

            cJSON* header_name = cJSON_CreateString(request->headers.headers[j].name);
            cJSON* header_value = cJSON_CreateString(request->headers.headers[j].value);
            cJSON* header_enabled = cJSON_CreateBool(true); 

            if (!header_name || !header_value || !header_enabled) {
                cJSON_Delete(header_obj);
                cJSON_Delete(json_request);
                cJSON_Delete(json);
                return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
            }

            cJSON_AddItemToObject(header_obj, "name", header_name);
            cJSON_AddItemToObject(header_obj, "value", header_value);
            cJSON_AddItemToObject(header_obj, "enabled", header_enabled);
            cJSON_AddItemToArray(req_headers, header_obj);
        }
        cJSON_AddItemToObject(json_request, "headers", req_headers);

        cJSON* req_body = cJSON_CreateObject();
        if (!req_body) {
            cJSON_Delete(json_request);
            cJSON_Delete(json);
            return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
        }

        cJSON* body_type = cJSON_CreateString("raw");

        if (request->body) {
            printf("DEBUG: Saving body content: '%s'\n", request->body);
        }

        cJSON* body_content = NULL;
        if (request->body && strlen(request->body) > 0) {

            bool is_multipart_form = (strstr(request->body, "----TinyRequestFormBoundary") != NULL) ||
                                   (strstr(request->body, "Content-Disposition: form-data") != NULL);
            bool is_urlencoded_form = (strstr(request->body, "=") != NULL && strstr(request->body, "&") != NULL &&
                                     !is_multipart_form);

            if (is_multipart_form || is_urlencoded_form) {

                body_content = cJSON_CreateString(request->body);
                printf("DEBUG: Saving form data content: '%s'\n", request->body);
            } else {

                const char* trimmed = request->body;
                while (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n' || *trimmed == '\r') {
                    trimmed++;
                }

                if ((*trimmed == '{' || *trimmed == '[') && !is_multipart_form && !is_urlencoded_form) {
                    cJSON* json_test = cJSON_Parse(request->body);
                    if (json_test) {

                        body_content = cJSON_CreateRaw(request->body);
                        cJSON_Delete(json_test);
                    } else {

                        body_content = cJSON_CreateString(request->body);
                    }
                } else {

                    body_content = cJSON_CreateString(request->body);
                }
            }
        } else {
            body_content = cJSON_CreateString("");
        }

        if (!body_type || !body_content) {
            cJSON_Delete(req_body);
            cJSON_Delete(json_request);
            cJSON_Delete(json);
            return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
        }

        cJSON_AddItemToObject(req_body, "type", body_type);
        cJSON_AddItemToObject(req_body, "content", body_content);
        cJSON_AddItemToObject(json_request, "body", req_body);

        // Save per-request authentication data
        cJSON* req_auth = cJSON_CreateObject();
        if (req_auth) {
            cJSON* auth_type = cJSON_CreateNumber(request->selected_auth_type);
            cJSON_AddItemToObject(req_auth, "type", auth_type);
            
            // Save authentication checkbox states
            cJSON* auth_api_key_enabled = cJSON_CreateBool(request->auth_api_key_enabled);
            cJSON* auth_bearer_enabled = cJSON_CreateBool(request->auth_bearer_enabled);
            cJSON* auth_basic_enabled = cJSON_CreateBool(request->auth_basic_enabled);
            cJSON* auth_oauth_enabled = cJSON_CreateBool(request->auth_oauth_enabled);
            
            if (auth_api_key_enabled && auth_bearer_enabled && auth_basic_enabled && auth_oauth_enabled) {
                cJSON_AddItemToObject(req_auth, "api_key_enabled", auth_api_key_enabled);
                cJSON_AddItemToObject(req_auth, "bearer_enabled", auth_bearer_enabled);
                cJSON_AddItemToObject(req_auth, "basic_enabled", auth_basic_enabled);
                cJSON_AddItemToObject(req_auth, "oauth_enabled", auth_oauth_enabled);
            }
            
            // Save authentication data based on type
            if (request->selected_auth_type == 1) { // API Key
                cJSON* api_key_name = cJSON_CreateString(request->auth_api_key_name);
                cJSON* api_key_value = cJSON_CreateString(request->auth_api_key_value);
                cJSON* api_key_location = cJSON_CreateNumber((double)request->auth_api_key_location);
                if (api_key_name && api_key_value && api_key_location) {
                    cJSON_AddItemToObject(req_auth, "api_key_name", api_key_name);
                    cJSON_AddItemToObject(req_auth, "api_key_value", api_key_value);
                    cJSON_AddItemToObject(req_auth, "api_key_location", api_key_location);
                }
            } else if (request->selected_auth_type == 2) { // Bearer Token
                cJSON* bearer_token = cJSON_CreateString(request->auth_bearer_token);
                if (bearer_token) {
                    cJSON_AddItemToObject(req_auth, "bearer_token", bearer_token);
                }
            } else if (request->selected_auth_type == 3) { // Basic Auth
                cJSON* basic_username = cJSON_CreateString(request->auth_basic_username);
                cJSON* basic_password = cJSON_CreateString(request->auth_basic_password);
                if (basic_username && basic_password) {
                    cJSON_AddItemToObject(req_auth, "basic_username", basic_username);
                    cJSON_AddItemToObject(req_auth, "basic_password", basic_password);
                }
            } else if (request->selected_auth_type == 4) { // OAuth 2.0
                cJSON* oauth_token = cJSON_CreateString(request->auth_oauth_token);
                if (oauth_token) {
                    cJSON_AddItemToObject(req_auth, "oauth_token", oauth_token);
                }
            }
            
            cJSON_AddItemToObject(json_request, "auth", req_auth);
        }

        cJSON_AddItemToArray(json_requests, json_request);
    }
    cJSON_AddItemToObject(json, "requests", json_requests);

    /* convert json to string */
    char* json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    /* write json to file */
    FILE* file = fopen(filepath, "w");
    if (!file) {
        free(json_string);
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    size_t json_len = strlen(json_string);
    size_t written = fwrite(json_string, 1, json_len, file);
    fclose(file);
    free(json_string);

    return (written == json_len) ? PERSISTENCE_SUCCESS : PERSISTENCE_ERROR_DISK_FULL;
}

int persistence_load_collection_new(Collection* collection, const char* filepath) {
    if (!collection || !filepath) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    int validation_result = persistence_validate_collection_file(filepath);
    if (validation_result != PERSISTENCE_SUCCESS) {
        if (validation_result == PERSISTENCE_ERROR_CORRUPTED_DATA || 
            validation_result == PERSISTENCE_ERROR_INVALID_JSON) {
            return persistence_handle_corrupted_file(filepath, "load collection");
        }
        return validation_result;
    }

    FILE* file = fopen(filepath, "rb");
    if (!file) {
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        return PERSISTENCE_ERROR_CORRUPTED_DATA;
    }

    char* json_string = (char*)malloc(file_size + 1);
    if (!json_string) {
        fclose(file);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    size_t read_size = fread(json_string, 1, file_size, file);
    json_string[read_size] = '\0';
    fclose(file);

    if (read_size != (size_t)file_size) {
        free(json_string);
        return PERSISTENCE_ERROR_CORRUPTED_DATA;
    }

    cJSON* json = cJSON_Parse(json_string);
    free(json_string);

    if (!json) {
        return PERSISTENCE_ERROR_INVALID_JSON;
    }

    collection_cleanup(collection);

    cJSON* json_id = cJSON_GetObjectItem(json, "id");
    cJSON* json_name = cJSON_GetObjectItem(json, "name");
    cJSON* json_description = cJSON_GetObjectItem(json, "description");
    cJSON* json_created_at = cJSON_GetObjectItem(json, "created_at");
    cJSON* json_modified_at = cJSON_GetObjectItem(json, "modified_at");

    collection_init(collection, "Untitled Collection", "");

    if (json_id && cJSON_IsString(json_id)) {
        strncpy(collection->id, json_id->valuestring, sizeof(collection->id) - 1);
        collection->id[sizeof(collection->id) - 1] = '\0';
    }

    if (json_name && cJSON_IsString(json_name)) {
        strncpy(collection->name, json_name->valuestring, sizeof(collection->name) - 1);
        collection->name[sizeof(collection->name) - 1] = '\0';
    }

    if (json_description && cJSON_IsString(json_description)) {
        strncpy(collection->description, json_description->valuestring, sizeof(collection->description) - 1);
        collection->description[sizeof(collection->description) - 1] = '\0';
    }

    if (json_created_at && cJSON_IsNumber(json_created_at)) {
        collection->created_at = (time_t)json_created_at->valuedouble;
    }

    if (json_modified_at && cJSON_IsNumber(json_modified_at)) {
        collection->modified_at = (time_t)json_modified_at->valuedouble;
    }

    cJSON* json_requests = cJSON_GetObjectItem(json, "requests");
    if (json_requests && cJSON_IsArray(json_requests)) {
        cJSON* json_request = NULL;
        cJSON_ArrayForEach(json_request, json_requests) {
            if (!cJSON_IsObject(json_request)) {
                continue;
            }

            Request temp_request;
            request_init(&temp_request);

            char request_name[256] = "Unnamed Request";
            cJSON* req_name = cJSON_GetObjectItem(json_request, "name");
            if (req_name && cJSON_IsString(req_name)) {
                strncpy(request_name, req_name->valuestring, sizeof(request_name) - 1);
                request_name[sizeof(request_name) - 1] = '\0';
            }

            cJSON* req_method = cJSON_GetObjectItem(json_request, "method");
            if (req_method && cJSON_IsString(req_method)) {
                strncpy(temp_request.method, req_method->valuestring, sizeof(temp_request.method) - 1);
                temp_request.method[sizeof(temp_request.method) - 1] = '\0';
            }

            cJSON* req_url = cJSON_GetObjectItem(json_request, "url");
            if (req_url && cJSON_IsString(req_url)) {
                strncpy(temp_request.url, req_url->valuestring, sizeof(temp_request.url) - 1);
                temp_request.url[sizeof(temp_request.url) - 1] = '\0';
            }

            cJSON* req_headers = cJSON_GetObjectItem(json_request, "headers");
            if (req_headers && cJSON_IsArray(req_headers)) {
                cJSON* header_item = NULL;
                cJSON_ArrayForEach(header_item, req_headers) {
                    if (cJSON_IsObject(header_item)) {
                        cJSON* header_name = cJSON_GetObjectItem(header_item, "name");
                        cJSON* header_value = cJSON_GetObjectItem(header_item, "value");
                        cJSON* header_enabled = cJSON_GetObjectItem(header_item, "enabled");

                        bool enabled = true;
                        if (header_enabled && cJSON_IsBool(header_enabled)) {
                            enabled = cJSON_IsTrue(header_enabled);
                        }

                        if (enabled && header_name && cJSON_IsString(header_name) &&
                            header_value && cJSON_IsString(header_value)) {
                            header_list_add(&temp_request.headers, header_name->valuestring, header_value->valuestring);
                        }
                    }
                }
            }

            cJSON* req_body = cJSON_GetObjectItem(json_request, "body");
            if (req_body && cJSON_IsObject(req_body)) {
                cJSON* body_content = cJSON_GetObjectItem(req_body, "content");
                if (body_content) {
                    if (cJSON_IsString(body_content)) {

                        const char* content = body_content->valuestring;
                        if (content && strlen(content) > 0) {
                            printf("DEBUG: Loading string body content: '%s'\n", content);
                            request_set_body(&temp_request, content, strlen(content));
                        }
                    } else if (cJSON_IsObject(body_content) || cJSON_IsArray(body_content)) {

                        char* json_string = cJSON_PrintUnformatted(body_content);
                        if (json_string) {
                            printf("DEBUG: Loading JSON body content: '%s'\n", json_string);
                            request_set_body(&temp_request, json_string, strlen(json_string));
                            free(json_string);
                        }
                    }
                }
            }

            // Load per-request authentication data
            cJSON* req_auth = cJSON_GetObjectItem(json_request, "auth");
            printf("DEBUG: Loading request '%s' - auth object found: %s\n",
                   request_name, req_auth ? "yes" : "no");
            if (req_auth && cJSON_IsObject(req_auth)) {
                cJSON* auth_type = cJSON_GetObjectItem(req_auth, "type");
                if (auth_type && cJSON_IsNumber(auth_type)) {
                    temp_request.selected_auth_type = (int)auth_type->valuedouble;
                    printf("DEBUG: Loaded auth type: %d for request '%s'\n",
                           temp_request.selected_auth_type, request_name);
                    
                    // Load authentication checkbox states (default to true for backward compatibility)
                    cJSON* auth_api_key_enabled = cJSON_GetObjectItem(req_auth, "api_key_enabled");
                    cJSON* auth_bearer_enabled = cJSON_GetObjectItem(req_auth, "bearer_enabled");
                    cJSON* auth_basic_enabled = cJSON_GetObjectItem(req_auth, "basic_enabled");
                    cJSON* auth_oauth_enabled = cJSON_GetObjectItem(req_auth, "oauth_enabled");

                    temp_request.auth_api_key_enabled = auth_api_key_enabled && cJSON_IsBool(auth_api_key_enabled) ?
                                                       cJSON_IsTrue(auth_api_key_enabled) : true;
                    temp_request.auth_bearer_enabled = auth_bearer_enabled && cJSON_IsBool(auth_bearer_enabled) ?
                                                      cJSON_IsTrue(auth_bearer_enabled) : true;
                    temp_request.auth_basic_enabled = auth_basic_enabled && cJSON_IsBool(auth_basic_enabled) ?
                                                     cJSON_IsTrue(auth_basic_enabled) : true;
                    temp_request.auth_oauth_enabled = auth_oauth_enabled && cJSON_IsBool(auth_oauth_enabled) ?
                                                     cJSON_IsTrue(auth_oauth_enabled) : true;
                    
                    printf("DEBUG: Loaded checkboxes for request '%s' - api_key: %s, bearer: %s, basic: %s, oauth: %s\n",
                           request_name,
                           temp_request.auth_api_key_enabled ? "true" : "false",
                           temp_request.auth_bearer_enabled ? "true" : "false",
                           temp_request.auth_basic_enabled ? "true" : "false",
                           temp_request.auth_oauth_enabled ? "true" : "false");
                    
                    // Load authentication data based on type
                    if (temp_request.selected_auth_type == 1) { // API Key
                        cJSON* api_key_name = cJSON_GetObjectItem(req_auth, "api_key_name");
                        cJSON* api_key_value = cJSON_GetObjectItem(req_auth, "api_key_value");
                        cJSON* api_key_location = cJSON_GetObjectItem(req_auth, "api_key_location");

                        if (api_key_name && cJSON_IsString(api_key_name)) {
                            strncpy(temp_request.auth_api_key_name, api_key_name->valuestring, sizeof(temp_request.auth_api_key_name) - 1);
                            temp_request.auth_api_key_name[sizeof(temp_request.auth_api_key_name) - 1] = '\0';
                        }
                        if (api_key_value && cJSON_IsString(api_key_value)) {
                            strncpy(temp_request.auth_api_key_value, api_key_value->valuestring, sizeof(temp_request.auth_api_key_value) - 1);
                            temp_request.auth_api_key_value[sizeof(temp_request.auth_api_key_value) - 1] = '\0';
                        }
                        if (api_key_location && cJSON_IsNumber(api_key_location)) {
                            temp_request.auth_api_key_location = (int)api_key_location->valuedouble;
                        }
                    } else if (temp_request.selected_auth_type == 2) { // Bearer Token
                        cJSON* bearer_token = cJSON_GetObjectItem(req_auth, "bearer_token");
                        if (bearer_token && cJSON_IsString(bearer_token)) {
                            strncpy(temp_request.auth_bearer_token, bearer_token->valuestring, sizeof(temp_request.auth_bearer_token) - 1);
                            temp_request.auth_bearer_token[sizeof(temp_request.auth_bearer_token) - 1] = '\0';
                        }
                    } else if (temp_request.selected_auth_type == 3) { // Basic Auth
                        cJSON* basic_username = cJSON_GetObjectItem(req_auth, "basic_username");
                        cJSON* basic_password = cJSON_GetObjectItem(req_auth, "basic_password");

                        if (basic_username && cJSON_IsString(basic_username)) {
                            strncpy(temp_request.auth_basic_username, basic_username->valuestring, sizeof(temp_request.auth_basic_username) - 1);
                            temp_request.auth_basic_username[sizeof(temp_request.auth_basic_username) - 1] = '\0';
                        }
                        if (basic_password && cJSON_IsString(basic_password)) {
                            strncpy(temp_request.auth_basic_password, basic_password->valuestring, sizeof(temp_request.auth_basic_password) - 1);
                            temp_request.auth_basic_password[sizeof(temp_request.auth_basic_password) - 1] = '\0';
                        }
                    } else if (temp_request.selected_auth_type == 4) { // OAuth 2.0
                        cJSON* oauth_token = cJSON_GetObjectItem(req_auth, "oauth_token");
                        if (oauth_token && cJSON_IsString(oauth_token)) {
                            strncpy(temp_request.auth_oauth_token, oauth_token->valuestring, sizeof(temp_request.auth_oauth_token) - 1);
                            temp_request.auth_oauth_token[sizeof(temp_request.auth_oauth_token) - 1] = '\0';
                        }
                    }
                }
            } else {
                // No auth object found - initialize with defaults (checkboxes enabled for backward compatibility)
                printf("DEBUG: No auth object found for request '%s' - using defaults\n", request_name);
                temp_request.selected_auth_type = 0;
                memset(temp_request.auth_api_key_name, 0, sizeof(temp_request.auth_api_key_name));
                memset(temp_request.auth_api_key_value, 0, sizeof(temp_request.auth_api_key_value));
                memset(temp_request.auth_bearer_token, 0, sizeof(temp_request.auth_bearer_token));
                memset(temp_request.auth_basic_username, 0, sizeof(temp_request.auth_basic_username));
                memset(temp_request.auth_basic_password, 0, sizeof(temp_request.auth_basic_password));
                memset(temp_request.auth_oauth_token, 0, sizeof(temp_request.auth_oauth_token));
                temp_request.auth_api_key_location = 0;
                // Default checkboxes to true for backward compatibility
                temp_request.auth_api_key_enabled = true;
                temp_request.auth_bearer_enabled = true;
                temp_request.auth_basic_enabled = true;
                temp_request.auth_oauth_enabled = true;
            }

            if (collection_add_request(collection, &temp_request, request_name) < 0) {
                request_cleanup(&temp_request);
            } else {
                printf("DEBUG: Successfully added request '%s' to collection with auth type: %d\n",
                       request_name, temp_request.selected_auth_type);
                request_cleanup(&temp_request);
            }
        }
    }

    cJSON_Delete(json);
    return PERSISTENCE_SUCCESS;
}

int persistence_load_collection_with_auth(Collection* collection, const char* filepath, void* app_state) {
    if (!collection || !filepath) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    AppState* state = (AppState*)app_state;

    int result = persistence_load_collection_new(collection, filepath);
    if (result != PERSISTENCE_SUCCESS) {
        return result;
    }

    if (!state) {
        return PERSISTENCE_SUCCESS; 
    }

    FILE* file = fopen(filepath, "rb");
    if (!file) {
        return PERSISTENCE_SUCCESS; 
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        return PERSISTENCE_SUCCESS;
    }

    char* json_string = (char*)malloc(file_size + 1);
    if (!json_string) {
        fclose(file);
        return PERSISTENCE_SUCCESS;
    }

    size_t read_size = fread(json_string, 1, file_size, file);
    json_string[read_size] = '\0';
    fclose(file);

    if (read_size != (size_t)file_size) {
        free(json_string);
        return PERSISTENCE_SUCCESS;
    }

    cJSON* json = cJSON_Parse(json_string);
    free(json_string);

    if (!json) {
        return PERSISTENCE_SUCCESS;
    }

    cJSON* collection_auth = cJSON_GetObjectItem(json, "auth");
    if (collection_auth && cJSON_IsObject(collection_auth)) {
        cJSON* auth_type = cJSON_GetObjectItem(collection_auth, "type");
        if (auth_type && cJSON_IsNumber(auth_type)) {
            state->selected_auth_type = (int)auth_type->valuedouble;

            // Load authentication checkbox states (default to true for backward compatibility)
            cJSON* auth_api_key_enabled = cJSON_GetObjectItem(collection_auth, "api_key_enabled");
            cJSON* auth_bearer_enabled = cJSON_GetObjectItem(collection_auth, "bearer_enabled");
            cJSON* auth_basic_enabled = cJSON_GetObjectItem(collection_auth, "basic_enabled");
            cJSON* auth_oauth_enabled = cJSON_GetObjectItem(collection_auth, "oauth_enabled");

            if (auth_api_key_enabled && cJSON_IsBool(auth_api_key_enabled)) {
                state->auth_api_key_enabled = cJSON_IsTrue(auth_api_key_enabled);
            } else {
                state->auth_api_key_enabled = true; // Default to enabled for backward compatibility
            }

            if (auth_bearer_enabled && cJSON_IsBool(auth_bearer_enabled)) {
                state->auth_bearer_enabled = cJSON_IsTrue(auth_bearer_enabled);
            } else {
                state->auth_bearer_enabled = true; // Default to enabled for backward compatibility
            }

            if (auth_basic_enabled && cJSON_IsBool(auth_basic_enabled)) {
                state->auth_basic_enabled = cJSON_IsTrue(auth_basic_enabled);
            } else {
                state->auth_basic_enabled = true; // Default to enabled for backward compatibility
            }

            if (auth_oauth_enabled && cJSON_IsBool(auth_oauth_enabled)) {
                state->auth_oauth_enabled = cJSON_IsTrue(auth_oauth_enabled);
            } else {
                state->auth_oauth_enabled = true; // Default to enabled for backward compatibility
            }

            if (state->selected_auth_type == 1) {
                cJSON* api_key_name = cJSON_GetObjectItem(collection_auth, "api_key_name");
                cJSON* api_key_value = cJSON_GetObjectItem(collection_auth, "api_key_value");
                cJSON* api_key_location = cJSON_GetObjectItem(collection_auth, "api_key_location");

                if (api_key_name && cJSON_IsString(api_key_name)) {
                    strncpy(state->auth_api_key_name, api_key_name->valuestring, sizeof(state->auth_api_key_name) - 1);
                    state->auth_api_key_name[sizeof(state->auth_api_key_name) - 1] = '\0';
                }
                if (api_key_value && cJSON_IsString(api_key_value)) {
                    strncpy(state->auth_api_key_value, api_key_value->valuestring, sizeof(state->auth_api_key_value) - 1);
                    state->auth_api_key_value[sizeof(state->auth_api_key_value) - 1] = '\0';
                }
                if (api_key_location && cJSON_IsNumber(api_key_location)) {
                    state->auth_api_key_location = (int)api_key_location->valuedouble;
                    printf("DEBUG: Loaded API key location: %d (from JSON: %f)\n",
                           state->auth_api_key_location, api_key_location->valuedouble);
                }
            } else if (state->selected_auth_type == 2) {
                cJSON* bearer_token = cJSON_GetObjectItem(collection_auth, "bearer_token");
                if (bearer_token && cJSON_IsString(bearer_token)) {
                    strncpy(state->auth_bearer_token, bearer_token->valuestring, sizeof(state->auth_bearer_token) - 1);
                    state->auth_bearer_token[sizeof(state->auth_bearer_token) - 1] = '\0';
                }
            } else if (state->selected_auth_type == 3) {
                cJSON* basic_username = cJSON_GetObjectItem(collection_auth, "basic_username");
                cJSON* basic_password = cJSON_GetObjectItem(collection_auth, "basic_password");

                if (basic_username && cJSON_IsString(basic_username)) {
                    strncpy(state->auth_basic_username, basic_username->valuestring, sizeof(state->auth_basic_username) - 1);
                    state->auth_basic_username[sizeof(state->auth_basic_username) - 1] = '\0';
                }
                if (basic_password && cJSON_IsString(basic_password)) {
                    strncpy(state->auth_basic_password, basic_password->valuestring, sizeof(state->auth_basic_password) - 1);
                    state->auth_basic_password[sizeof(state->auth_basic_password) - 1] = '\0';
                }
            } else if (state->selected_auth_type == 4) {
                cJSON* oauth_token = cJSON_GetObjectItem(collection_auth, "oauth_token");
                if (oauth_token && cJSON_IsString(oauth_token)) {
                    strncpy(state->auth_oauth_token, oauth_token->valuestring, sizeof(state->auth_oauth_token) - 1);
                    state->auth_oauth_token[sizeof(state->auth_oauth_token) - 1] = '\0';
                }
            }
        }
    }

    cJSON* collection_cookies = cJSON_GetObjectItem(json, "cookies");
    printf("DEBUG: Loading cookies from persistence - found cookies array: %s\n",
           collection_cookies ? "yes" : "no");

    if (collection_cookies && cJSON_IsArray(collection_cookies)) {
        int cookie_array_size = cJSON_GetArraySize(collection_cookies);
        printf("DEBUG: Cookie array has %d items\n", cookie_array_size);

        collection->cookie_jar.count = 0;

        cJSON* cookie_item = NULL;
        cJSON_ArrayForEach(cookie_item, collection_cookies) {
            if (cJSON_IsObject(cookie_item) && collection->cookie_jar.count < collection->cookie_jar.capacity) {
                StoredCookie* cookie = &collection->cookie_jar.cookies[collection->cookie_jar.count];

                memset(cookie, 0, sizeof(StoredCookie));
                strncpy(cookie->domain, "", sizeof(cookie->domain) - 1);
                cookie->domain[sizeof(cookie->domain) - 1] = '\0';
                strncpy(cookie->path, "/", sizeof(cookie->path) - 1);
                cookie->path[sizeof(cookie->path) - 1] = '\0';

                cJSON* cookie_name = cJSON_GetObjectItem(cookie_item, "name");
                cJSON* cookie_value = cJSON_GetObjectItem(cookie_item, "value");
                cJSON* cookie_domain = cJSON_GetObjectItem(cookie_item, "domain");
                cJSON* cookie_path = cJSON_GetObjectItem(cookie_item, "path");
                cJSON* cookie_expires = cJSON_GetObjectItem(cookie_item, "expires");
                cJSON* cookie_max_age = cJSON_GetObjectItem(cookie_item, "max_age");
                cJSON* cookie_secure = cJSON_GetObjectItem(cookie_item, "secure");
                cJSON* cookie_http_only = cJSON_GetObjectItem(cookie_item, "http_only");
                cJSON* cookie_same_site_strict = cJSON_GetObjectItem(cookie_item, "same_site_strict");
                cJSON* cookie_same_site_lax = cJSON_GetObjectItem(cookie_item, "same_site_lax");
                cJSON* cookie_created_at = cJSON_GetObjectItem(cookie_item, "created_at");

                if (cookie_name && cJSON_IsString(cookie_name)) {
                    strncpy(cookie->name, cookie_name->valuestring, sizeof(cookie->name) - 1);
                    cookie->name[sizeof(cookie->name) - 1] = '\0';
                }
                if (cookie_value && cJSON_IsString(cookie_value)) {
                    strncpy(cookie->value, cookie_value->valuestring, sizeof(cookie->value) - 1);
                    cookie->value[sizeof(cookie->value) - 1] = '\0';
                }
                if (cookie_domain && cJSON_IsString(cookie_domain)) {
                    strncpy(cookie->domain, cookie_domain->valuestring, sizeof(cookie->domain) - 1);
                    cookie->domain[sizeof(cookie->domain) - 1] = '\0';
                }
                if (cookie_path && cJSON_IsString(cookie_path)) {
                    strncpy(cookie->path, cookie_path->valuestring, sizeof(cookie->path) - 1);
                    cookie->path[sizeof(cookie->path) - 1] = '\0';
                }
                if (cookie_expires && cJSON_IsNumber(cookie_expires)) {
                    cookie->expires = (time_t)cookie_expires->valuedouble;
                }
                if (cookie_max_age && cJSON_IsNumber(cookie_max_age)) {
                    cookie->max_age = (int)cookie_max_age->valuedouble;
                }
                if (cookie_secure && cJSON_IsBool(cookie_secure)) {
                    cookie->secure = cJSON_IsTrue(cookie_secure);
                }
                if (cookie_http_only && cJSON_IsBool(cookie_http_only)) {
                    cookie->http_only = cJSON_IsTrue(cookie_http_only);
                }
                if (cookie_same_site_strict && cJSON_IsBool(cookie_same_site_strict)) {
                    cookie->same_site_strict = cJSON_IsTrue(cookie_same_site_strict);
                }
                if (cookie_same_site_lax && cJSON_IsBool(cookie_same_site_lax)) {
                    cookie->same_site_lax = cJSON_IsTrue(cookie_same_site_lax);
                }
                if (cookie_created_at && cJSON_IsNumber(cookie_created_at)) {
                    cookie->created_at = (time_t)cookie_created_at->valuedouble;
                }

                if (strlen(cookie->name) > 0) {
                    collection->cookie_jar.count++;
                    printf("DEBUG: Loaded cookie: %s=%s (domain: %s, path: %s)\n",
                           cookie->name, cookie->value, cookie->domain, cookie->path);
                }
            }
        }
        printf("DEBUG: Loaded %d cookies from collection file\n", collection->cookie_jar.count);
    }

    cJSON_Delete(json);
    return PERSISTENCE_SUCCESS;
}

int persistence_export_collection(const Collection* collection, const char* filepath) {

    return persistence_save_collection_new(collection, filepath);
}

int persistence_import_collection(Collection* collection, const char* filepath) {

    return persistence_load_collection_new(collection, filepath);
}

int persistence_save_all_collections(const CollectionManager* manager) {
    if (!manager) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    if (persistence_create_config_dir() != 0) {
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    if (persistence_create_collections_dir() != 0) {
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    for (int i = 0; i < manager->count; i++) {
        const Collection* collection = &manager->collections[i];

        char filename[128];
        snprintf(filename, sizeof(filename), "%s.json", collection->id);

        char* filepath = persistence_get_collections_path(filename);
        if (!filepath) {
            return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
        }

        int result = persistence_save_collection_new(collection, filepath);
        free(filepath);

        if (result != PERSISTENCE_SUCCESS) {
            return result;
        }
    }

    return persistence_save_collection_manager_state(manager);
}

int persistence_save_all_collections_with_auth(const CollectionManager* manager, const void* app_state) {
    if (!manager) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    if (persistence_create_config_dir() != 0) {
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    if (persistence_create_collections_dir() != 0) {
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    for (int i = 0; i < manager->count; i++) {
        const Collection* collection = &manager->collections[i];

        char filename[128];
        snprintf(filename, sizeof(filename), "%s.json", collection->id);

        char* filepath = persistence_get_collections_path(filename);
        if (!filepath) {
            return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
        }

        int result = persistence_save_collection_with_auth(collection, filepath, app_state);
        free(filepath);

        if (result != PERSISTENCE_SUCCESS) {
            return result;
        }
    }

    return persistence_save_collection_manager_state(manager);
}

int persistence_delete_collection_file(const char* collection_id) {
    if (!collection_id) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    char filename[128];
    snprintf(filename, sizeof(filename), "%s.json", collection_id);

    char* filepath = persistence_get_collections_path(filename);
    if (!filepath) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    if (!persistence_file_exists(filepath)) {
        free(filepath);
        return PERSISTENCE_SUCCESS; 
    }

    int result = remove(filepath);
    free(filepath);

    if (result == 0) {
        printf("Deleted collection file: %s\n", filename);
        return PERSISTENCE_SUCCESS;
    } else {
        printf("Error: %s\n", persistence_get_user_friendly_error(PERSISTENCE_ERROR_PERMISSION_DENIED, "delete collection file"));
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }
}

int persistence_load_all_collections(CollectionManager* manager) {
    if (!manager) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    persistence_load_collection_manager_state(manager);

    if (persistence_create_collections_dir() != 0) {
        return PERSISTENCE_SUCCESS; 
    }

    char* collections_dir = persistence_get_collections_path("");
    if (!collections_dir) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    size_t len = strlen(collections_dir);
    if (len > 0 && (collections_dir[len-1] == '/' || collections_dir[len-1] == '\\')) {
        collections_dir[len-1] = '\0';
    }

    printf("Loading collections from directory: %s\n", collections_dir);

#ifdef _WIN32
    WIN32_FIND_DATAA find_data;
    char search_pattern[1024];
    snprintf(search_pattern, sizeof(search_pattern), "%s\\*.json", collections_dir);

    HANDLE find_handle = FindFirstFileA(search_pattern, &find_data);
    if (find_handle != INVALID_HANDLE_VALUE) {
        do {
            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                char filepath[1024];
                snprintf(filepath, sizeof(filepath), "%s\\%s", collections_dir, find_data.cFileName);

                printf("Loading collection from: %s\n", filepath);
                Collection temp_collection;
                if (persistence_load_collection_new(&temp_collection, filepath) == PERSISTENCE_SUCCESS) {

                    bool is_duplicate = false;
                    for (int i = 0; i < manager->count; i++) {
                        if (strcmp(manager->collections[i].id, temp_collection.id) == 0) {
                            printf("Skipping duplicate collection: %s\n", temp_collection.name);
                            is_duplicate = true;
                            break;
                        }
                    }

                    if (!is_duplicate) {
                        collection_manager_add_collection(manager, &temp_collection);
                        printf("Added collection: %s\n", temp_collection.name);
                    }
                    collection_cleanup(&temp_collection);
                }
            }
        } while (FindNextFileA(find_handle, &find_data));
        FindClose(find_handle);
    }
#else

    #include <dirent.h>
    DIR* dir = opendir(collections_dir);
    if (dir != NULL) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {

            if (strstr(entry->d_name, ".json") != NULL) {
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", collections_dir, entry->d_name);
                struct stat file_stat;
                if (stat(full_path, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
                    printf("Loading collection from: %s\n", full_path);
                    Collection temp_collection;
                    memset(&temp_collection, 0, sizeof(Collection));
                    if (persistence_load_collection_new(&temp_collection, full_path) == PERSISTENCE_SUCCESS) {

                        bool is_duplicate = false;
                        for (int i = 0; i < manager->count; i++) {
                            if (strcmp(manager->collections[i].id, temp_collection.id) == 0) {
                                printf("Skipping duplicate collection: %s\n", temp_collection.name);
                                is_duplicate = true;
                                break;
                            }
                        }

                        if (!is_duplicate) {
                            collection_manager_add_collection(manager, &temp_collection);
                            printf("Added collection: %s\n", temp_collection.name);
                        }
                        collection_cleanup(&temp_collection);
                    }
                }
            }
        }
        closedir(dir);
    }
#endif

    free(collections_dir);
    printf("Loaded %d collections total\n", manager->count);
    return PERSISTENCE_SUCCESS;
}

int persistence_load_all_collections_with_auth(CollectionManager* manager, void* app_state) {
    if (!manager) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    AppState* state = (AppState*)app_state;

    persistence_load_collection_manager_state(manager);

    if (persistence_create_collections_dir() != 0) {
        return PERSISTENCE_SUCCESS; 
    }

    char* collections_dir = persistence_get_collections_path("");
    if (!collections_dir) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    size_t len = strlen(collections_dir);
    if (len > 0 && (collections_dir[len-1] == '/' || collections_dir[len-1] == '\\')) {
        collections_dir[len-1] = '\0';
    }

    printf("Loading collections with authentication from directory: %s\n", collections_dir);

    bool first_collection_loaded = false;

#ifdef _WIN32
    WIN32_FIND_DATAA find_data;
    char search_pattern[1024];
    snprintf(search_pattern, sizeof(search_pattern), "%s\\*.json", collections_dir);

    HANDLE find_handle = FindFirstFileA(search_pattern, &find_data);
    if (find_handle != INVALID_HANDLE_VALUE) {
        do {
            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                char filepath[1024];
                snprintf(filepath, sizeof(filepath), "%s\\%s", collections_dir, find_data.cFileName);

                printf("Loading collection from: %s\n", filepath);
                Collection temp_collection;

                int load_result;
                if (state) {

                    load_result = persistence_load_collection_with_auth(&temp_collection, filepath, state);

                    if (!first_collection_loaded && load_result == PERSISTENCE_SUCCESS) {
                        first_collection_loaded = true;
                    }
                } else {
                    load_result = persistence_load_collection_new(&temp_collection, filepath);
                }

                if (load_result == PERSISTENCE_SUCCESS) {

                    bool is_duplicate = false;
                    for (int i = 0; i < manager->count; i++) {
                        if (strcmp(manager->collections[i].id, temp_collection.id) == 0) {
                            printf("Skipping duplicate collection: %s\n", temp_collection.name);
                            is_duplicate = true;
                            break;
                        }
                    }

                    if (!is_duplicate) {
                        collection_manager_add_collection(manager, &temp_collection);
                        printf("Added collection: %s\n", temp_collection.name);
                    }
                    collection_cleanup(&temp_collection);
                }
            }
        } while (FindNextFileA(find_handle, &find_data));
        FindClose(find_handle);
    }
#else

    #include <dirent.h>
    DIR* dir = opendir(collections_dir);
    if (dir != NULL) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {

            if (strstr(entry->d_name, ".json") != NULL) {
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", collections_dir, entry->d_name);
                struct stat file_stat;
                if (stat(full_path, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
                    printf("Loading collection from: %s\n", full_path);
                    Collection temp_collection;
                    memset(&temp_collection, 0, sizeof(Collection));

                    int load_result;
                    if (state) {

                        load_result = persistence_load_collection_with_auth(&temp_collection, full_path, state);

                        if (!first_collection_loaded && load_result == PERSISTENCE_SUCCESS) {
                            first_collection_loaded = true;
                        }
                    } else {
                        load_result = persistence_load_collection_new(&temp_collection, full_path);
                    }

                    if (load_result == PERSISTENCE_SUCCESS) {

                        bool is_duplicate = false;
                        for (int i = 0; i < manager->count; i++) {
                            if (strcmp(manager->collections[i].id, temp_collection.id) == 0) {
                                printf("Skipping duplicate collection: %s\n", temp_collection.name);
                                is_duplicate = true;
                                break;
                            }
                        }

                        if (!is_duplicate) {
                            collection_manager_add_collection(manager, &temp_collection);
                            printf("Added collection: %s\n", temp_collection.name);
                        }
                        collection_cleanup(&temp_collection);
                    }
                }
            }
        }
        closedir(dir);
    }
#endif

    free(collections_dir);
    printf("Loaded %d collections total with authentication data\n", manager->count);
    return PERSISTENCE_SUCCESS;
}

int persistence_save_collection_manager_state(const CollectionManager* manager) {
    if (!manager) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    cJSON* json = cJSON_CreateObject();
    if (!json) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    cJSON* active_collection = cJSON_CreateNumber(manager->active_collection_index);
    cJSON* active_request = cJSON_CreateNumber(manager->active_request_index);
    cJSON* collection_count = cJSON_CreateNumber(manager->count);

    if (!active_collection || !active_request || !collection_count) {
        cJSON_Delete(json);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    cJSON_AddItemToObject(json, "active_collection_index", active_collection);
    cJSON_AddItemToObject(json, "active_request_index", active_request);
    cJSON_AddItemToObject(json, "collection_count", collection_count);

    cJSON* collection_ids = cJSON_CreateArray();
    if (!collection_ids) {
        cJSON_Delete(json);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    for (int i = 0; i < manager->count; i++) {
        cJSON* id = cJSON_CreateString(manager->collections[i].id);
        if (id) {
            cJSON_AddItemToArray(collection_ids, id);
        }
    }
    cJSON_AddItemToObject(json, "collection_ids", collection_ids);

    char* json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    char* filepath = persistence_get_config_path("collections_state.json");
    if (!filepath) {
        free(json_string);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    FILE* file = fopen(filepath, "w");
    if (!file) {
        free(json_string);
        free(filepath);
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    size_t json_len = strlen(json_string);
    size_t written = fwrite(json_string, 1, json_len, file);
    fclose(file);

    free(json_string);
    free(filepath);

    return (written == json_len) ? PERSISTENCE_SUCCESS : PERSISTENCE_ERROR_DISK_FULL;
}

int persistence_load_collection_manager_state(CollectionManager* manager) {
    if (!manager) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    char* filepath = persistence_get_config_path("collections_state.json");
    if (!filepath) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    if (!persistence_file_exists(filepath)) {
        free(filepath);
        return PERSISTENCE_ERROR_FILE_NOT_FOUND;
    }

    FILE* file = fopen(filepath, "rb");
    if (!file) {
        free(filepath);
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        free(filepath);
        return PERSISTENCE_ERROR_CORRUPTED_DATA;
    }

    char* json_string = (char*)malloc(file_size + 1);
    if (!json_string) {
        fclose(file);
        free(filepath);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    size_t read_size = fread(json_string, 1, file_size, file);
    json_string[read_size] = '\0';
    fclose(file);
    free(filepath);

    if (read_size != (size_t)file_size) {
        free(json_string);
        return PERSISTENCE_ERROR_CORRUPTED_DATA;
    }

    cJSON* json = cJSON_Parse(json_string);
    free(json_string);

    if (!json) {
        return PERSISTENCE_ERROR_INVALID_JSON;
    }

    cJSON* active_collection = cJSON_GetObjectItem(json, "active_collection_index");
    cJSON* active_request = cJSON_GetObjectItem(json, "active_request_index");

    if (active_collection && cJSON_IsNumber(active_collection)) {
        manager->active_collection_index = (int)active_collection->valuedouble;
    }

    if (active_request && cJSON_IsNumber(active_request)) {
        manager->active_request_index = (int)active_request->valuedouble;
    }

    cJSON_Delete(json);
    return PERSISTENCE_SUCCESS;
}

int persistence_auto_save_collections(const CollectionManager* manager) {
    if (!manager) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    int backup_result = persistence_create_auto_save_backup(manager);
    if (backup_result != PERSISTENCE_SUCCESS) {
        return backup_result;
    }

    return persistence_save_all_collections(manager);
}

int persistence_restore_from_auto_save(CollectionManager* manager) {
    if (!manager) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    char* backup_path = persistence_get_auto_save_path("collections_backup.json");
    if (!backup_path) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    if (!persistence_file_exists(backup_path)) {
        free(backup_path);
        return PERSISTENCE_ERROR_FILE_NOT_FOUND;
    }

    free(backup_path);
    return PERSISTENCE_SUCCESS;
}

int persistence_create_auto_save_backup(const CollectionManager* manager) {
    if (!manager) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    if (persistence_create_auto_save_dir() != 0) {
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    cJSON* json = cJSON_CreateObject();
    if (!json) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    cJSON* timestamp = cJSON_CreateNumber((double)time(NULL));
    if (!timestamp) {
        cJSON_Delete(json);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }
    cJSON_AddItemToObject(json, "backup_timestamp", timestamp);

    cJSON* collections_array = cJSON_CreateArray();
    if (!collections_array) {
        cJSON_Delete(json);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    for (int i = 0; i < manager->count; i++) {
        const Collection* collection = &manager->collections[i];

        cJSON* collection_json = cJSON_CreateObject();
        if (!collection_json) {
            cJSON_Delete(json);
            return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
        }

        cJSON* col_id = cJSON_CreateString(collection->id);
        cJSON* col_name = cJSON_CreateString(collection->name);
        cJSON* col_request_count = cJSON_CreateNumber(collection->request_count);

        if (!col_id || !col_name || !col_request_count) {
            cJSON_Delete(collection_json);
            cJSON_Delete(json);
            return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
        }

        cJSON_AddItemToObject(collection_json, "id", col_id);
        cJSON_AddItemToObject(collection_json, "name", col_name);
        cJSON_AddItemToObject(collection_json, "request_count", col_request_count);

        cJSON_AddItemToArray(collections_array, collection_json);
    }
    cJSON_AddItemToObject(json, "collections", collections_array);

    char* json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    char* backup_path = persistence_get_auto_save_path("collections_backup.json");
    if (!backup_path) {
        free(json_string);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    FILE* file = fopen(backup_path, "w");
    if (!file) {
        free(json_string);
        free(backup_path);
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    size_t json_len = strlen(json_string);
    size_t written = fwrite(json_string, 1, json_len, file);
    fclose(file);

    free(json_string);
    free(backup_path);

    return (written == json_len) ? PERSISTENCE_SUCCESS : PERSISTENCE_ERROR_DISK_FULL;
}

int persistence_cleanup_old_auto_saves(int keep_count) {

    return PERSISTENCE_SUCCESS;
}

int persistence_migrate_legacy_requests(CollectionManager* manager) {
    if (!manager) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    char* migration_marker_path = persistence_get_config_path("migration_completed.marker");
    if (!migration_marker_path) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    if (persistence_file_exists(migration_marker_path)) {
        free(migration_marker_path);
        return PERSISTENCE_SUCCESS; 
    }

    char* legacy_path = persistence_get_config_path("saved_requests.json");
    if (!legacy_path) {
        free(migration_marker_path);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    if (!persistence_file_exists(legacy_path)) {
        free(legacy_path);

        FILE* marker_file = fopen(migration_marker_path, "w");
        if (marker_file) {
            fprintf(marker_file, "Migration completed on first run - no legacy data found\n");
            fclose(marker_file);
        }
        free(migration_marker_path);
        return PERSISTENCE_SUCCESS; 
    }

    int validation_result = persistence_validate_collection_file(legacy_path);
    if (validation_result != PERSISTENCE_SUCCESS) {
        printf("Error: %s\n", persistence_get_user_friendly_error(validation_result, "migrate legacy data"));
        if (validation_result == PERSISTENCE_ERROR_CORRUPTED_DATA || 
            validation_result == PERSISTENCE_ERROR_INVALID_JSON) {
            persistence_handle_corrupted_file(legacy_path, "migrate legacy data");
        }
        free(legacy_path);
        free(migration_marker_path);
        return validation_result;
    }

    persistence_backup_legacy_data();

    int migrate_result = persistence_load_legacy_and_create_collection(manager, legacy_path);

    free(legacy_path);

    if (migrate_result == PERSISTENCE_SUCCESS) {
        FILE* marker_file = fopen(migration_marker_path, "w");
        if (marker_file) {
            fprintf(marker_file, "Migration completed successfully\n");
            fclose(marker_file);
            printf("Migration marker created\n");
        }
    }

    free(migration_marker_path);
    return migrate_result;
}

int persistence_load_legacy_and_create_collection(CollectionManager* manager, const char* legacy_path) {
    if (!manager || !legacy_path) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    FILE* file = fopen(legacy_path, "rb");
    if (!file) {
        return PERSISTENCE_ERROR_FILE_NOT_FOUND;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        return PERSISTENCE_ERROR_CORRUPTED_DATA;
    }

    char* json_string = (char*)malloc(file_size + 1);
    if (!json_string) {
        fclose(file);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    size_t read_size = fread(json_string, 1, file_size, file);
    json_string[read_size] = '\0';
    fclose(file);

    if (read_size != (size_t)file_size) {
        free(json_string);
        return PERSISTENCE_ERROR_CORRUPTED_DATA;
    }

    cJSON* json_array = cJSON_Parse(json_string);
    free(json_string);

    if (!json_array || !cJSON_IsArray(json_array)) {
        if (json_array) cJSON_Delete(json_array);
        return PERSISTENCE_ERROR_INVALID_JSON;
    }

    int request_count = cJSON_GetArraySize(json_array);
    if (request_count == 0) {
        cJSON_Delete(json_array);
        return PERSISTENCE_SUCCESS; 
    }

    Collection default_collection;
    collection_init(&default_collection, "Default Collection", "Migrated from legacy saved requests");

    cJSON* json_request = NULL;
    int migrated_count = 0;
    cJSON_ArrayForEach(json_request, json_array) {
        if (!cJSON_IsObject(json_request)) {
            continue;
        }

        Request temp_request;
        request_init(&temp_request);

        char request_name[256] = "Unnamed Request";
        cJSON* json_name = cJSON_GetObjectItem(json_request, "name");
        if (json_name && cJSON_IsString(json_name)) {
            strncpy(request_name, json_name->valuestring, sizeof(request_name) - 1);
            request_name[sizeof(request_name) - 1] = '\0';
        }

        cJSON* json_method = cJSON_GetObjectItem(json_request, "method");
        if (json_method && cJSON_IsString(json_method)) {
            strncpy(temp_request.method, json_method->valuestring, sizeof(temp_request.method) - 1);
            temp_request.method[sizeof(temp_request.method) - 1] = '\0';
        }

        cJSON* json_url = cJSON_GetObjectItem(json_request, "url");
        if (json_url && cJSON_IsString(json_url)) {
            strncpy(temp_request.url, json_url->valuestring, sizeof(temp_request.url) - 1);
            temp_request.url[sizeof(temp_request.url) - 1] = '\0';
        }

        cJSON* json_headers = cJSON_GetObjectItem(json_request, "headers");
        if (json_headers && cJSON_IsArray(json_headers)) {
            cJSON* header_item = NULL;
            cJSON_ArrayForEach(header_item, json_headers) {
                if (cJSON_IsObject(header_item)) {
                    cJSON* header_name = cJSON_GetObjectItem(header_item, "name");
                    cJSON* header_value = cJSON_GetObjectItem(header_item, "value");

                    if (header_name && cJSON_IsString(header_name) &&
                        header_value && cJSON_IsString(header_value)) {
                        header_list_add(&temp_request.headers, header_name->valuestring, header_value->valuestring);
                    }
                }
            }
        }

        cJSON* json_body = cJSON_GetObjectItem(json_request, "body");
        if (json_body && cJSON_IsString(json_body)) {
            const char* body_content = json_body->valuestring;
            if (strlen(body_content) > 0) {
                request_set_body(&temp_request, body_content, strlen(body_content));
            }
        }

        if (collection_add_request(&default_collection, &temp_request, request_name) >= 0) {
            migrated_count++;
        } else {
            printf("Warning: %s\n", persistence_get_user_friendly_error(PERSISTENCE_ERROR_MEMORY_ALLOCATION, "migrate request"));
        }

        request_cleanup(&temp_request);
    }

    cJSON_Delete(json_array);

    int result = collection_manager_add_collection(manager, &default_collection);
    collection_cleanup(&default_collection);

    if (result >= 0) {

        persistence_save_all_collections(manager);
        printf("Successfully migrated %d requests to Default Collection\n", migrated_count);
        return PERSISTENCE_SUCCESS;
    }

    return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
}

int persistence_create_default_collection_from_legacy(CollectionManager* manager, const void* legacy_collection) {

    return PERSISTENCE_ERROR_NULL_PARAM;
}

int persistence_backup_legacy_data(void) {

    char* legacy_path = persistence_get_config_path("saved_requests.json");
    if (!legacy_path) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    if (!persistence_file_exists(legacy_path)) {
        free(legacy_path);
        return PERSISTENCE_SUCCESS; 
    }

    char* backup_path = persistence_get_config_path("saved_requests_backup.json");
    if (!backup_path) {
        free(legacy_path);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    FILE* source = fopen(legacy_path, "rb");
    FILE* dest = fopen(backup_path, "wb");

    if (!source || !dest) {
        if (source) fclose(source);
        if (dest) fclose(dest);
        free(legacy_path);
        free(backup_path);
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, bytes, dest);
    }

    fclose(source);
    fclose(dest);
    free(legacy_path);
    free(backup_path);

    printf("Legacy data backed up to saved_requests_backup.json\n");
    return PERSISTENCE_SUCCESS;
}

int persistence_save_settings(const CollectionManager* manager, bool auto_save_enabled, int auto_save_interval) {

    cJSON* json = cJSON_CreateObject();
    if (!json) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    cJSON* ui_settings = cJSON_CreateObject();
    if (!ui_settings) {
        cJSON_Delete(json);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    cJSON* theme = cJSON_CreateString("gruvbox_dark");
    cJSON* active_tab = cJSON_CreateNumber(0);

    if (!theme || !active_tab) {
        cJSON_Delete(ui_settings);
        cJSON_Delete(json);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    cJSON_AddItemToObject(ui_settings, "theme", theme);
    cJSON_AddItemToObject(ui_settings, "active_tab", active_tab);
    cJSON_AddItemToObject(json, "ui", ui_settings);

    cJSON* collections_settings = cJSON_CreateObject();
    if (!collections_settings) {
        cJSON_Delete(json);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    cJSON* auto_save = cJSON_CreateBool(auto_save_enabled);
    cJSON* save_interval = cJSON_CreateNumber(auto_save_interval);

    if (!auto_save || !save_interval) {
        cJSON_Delete(collections_settings);
        cJSON_Delete(json);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    cJSON_AddItemToObject(collections_settings, "auto_save_enabled", auto_save);
    cJSON_AddItemToObject(collections_settings, "auto_save_interval", save_interval);

    if (manager && manager->active_collection_index >= 0) {
        const Collection* active_collection = &manager->collections[manager->active_collection_index];
        cJSON* last_active_collection = cJSON_CreateString(active_collection->id);
        if (last_active_collection) {
            cJSON_AddItemToObject(collections_settings, "last_active_collection", last_active_collection);
        }
    }

    cJSON_AddItemToObject(json, "collections", collections_settings);

    cJSON* http_settings = cJSON_CreateObject();
    if (!http_settings) {
        cJSON_Delete(json);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    cJSON* ssl_verify = cJSON_CreateBool(true);
    cJSON* timeout = cJSON_CreateNumber(30);
    cJSON* follow_redirects = cJSON_CreateBool(true);
    cJSON* max_redirects = cJSON_CreateNumber(5);

    if (!ssl_verify || !timeout || !follow_redirects || !max_redirects) {
        cJSON_Delete(http_settings);
        cJSON_Delete(json);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    cJSON_AddItemToObject(http_settings, "ssl_verify_enabled", ssl_verify);
    cJSON_AddItemToObject(http_settings, "timeout", timeout);
    cJSON_AddItemToObject(http_settings, "follow_redirects", follow_redirects);
    cJSON_AddItemToObject(http_settings, "max_redirects", max_redirects);
    cJSON_AddItemToObject(json, "http", http_settings);

    char* json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    char* filepath = persistence_get_config_path("settings.json");
    if (!filepath) {
        free(json_string);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    FILE* file = fopen(filepath, "w");
    if (!file) {
        free(json_string);
        free(filepath);
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    size_t json_len = strlen(json_string);
    size_t written = fwrite(json_string, 1, json_len, file);
    fclose(file);

    free(json_string);
    free(filepath);

    return (written == json_len) ? PERSISTENCE_SUCCESS : PERSISTENCE_ERROR_DISK_FULL;
}

int persistence_load_settings(bool* auto_save_enabled, int* auto_save_interval) {
    if (!auto_save_enabled || !auto_save_interval) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    *auto_save_enabled = true;
    *auto_save_interval = 300; 

    char* filepath = persistence_get_config_path("settings.json");
    if (!filepath) {
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    if (!persistence_file_exists(filepath)) {
        free(filepath);
        return PERSISTENCE_SUCCESS; 
    }

    FILE* file = fopen(filepath, "rb");
    if (!file) {
        free(filepath);
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        free(filepath);
        return PERSISTENCE_SUCCESS; 
    }

    char* json_string = (char*)malloc(file_size + 1);
    if (!json_string) {
        fclose(file);
        free(filepath);
        return PERSISTENCE_ERROR_MEMORY_ALLOCATION;
    }

    size_t read_size = fread(json_string, 1, file_size, file);
    json_string[read_size] = '\0';
    fclose(file);
    free(filepath);

    if (read_size != (size_t)file_size) {
        free(json_string);
        return PERSISTENCE_SUCCESS; 
    }

    cJSON* json = cJSON_Parse(json_string);
    free(json_string);

    if (!json) {
        return PERSISTENCE_SUCCESS; 
    }

    cJSON* collections_settings = cJSON_GetObjectItem(json, "collections");
    if (collections_settings && cJSON_IsObject(collections_settings)) {
        cJSON* auto_save = cJSON_GetObjectItem(collections_settings, "auto_save_enabled");
        cJSON* save_interval = cJSON_GetObjectItem(collections_settings, "auto_save_interval");

        if (auto_save && cJSON_IsBool(auto_save)) {
            *auto_save_enabled = cJSON_IsTrue(auto_save);
        }

        if (save_interval && cJSON_IsNumber(save_interval)) {
            *auto_save_interval = (int)save_interval->valuedouble;

            if (*auto_save_interval < 30) *auto_save_interval = 30;
            if (*auto_save_interval > 3600) *auto_save_interval = 3600;
        }
    }

    cJSON_Delete(json);
    return PERSISTENCE_SUCCESS;
}

int persistence_create_collections_dir(void) {
#ifdef _WIN32
    char* appdata_path = getenv("LOCALAPPDATA");
    if (!appdata_path) {
        appdata_path = getenv("APPDATA");
        if (!appdata_path) {
            return -1;
        }
    }

    char collections_dir[MAX_PATH];
    snprintf(collections_dir, sizeof(collections_dir), "%s\\TinyRequest\\collections", appdata_path);

    DWORD attrs = GetFileAttributesA(collections_dir);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryA(collections_dir, NULL)) {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS) {
                return -1;
            }
        }
    } else if (!(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return -1;
    }

    return 0;
#else
    char* home = getenv("HOME");
    if (!home) {
        return -1;
    }

    char collections_dir[1024];
    snprintf(collections_dir, sizeof(collections_dir), "%s/.config/tinyrequest/collections", home);

    if (mkdir(collections_dir, 0755) != 0) {
        struct stat st;
        if (stat(collections_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
            return -1;
        }
    }

    return 0;
#endif
}

int persistence_create_auto_save_dir(void) {
#ifdef _WIN32
    char* appdata_path = getenv("LOCALAPPDATA");
    if (!appdata_path) {
        appdata_path = getenv("APPDATA");
        if (!appdata_path) {
            return -1;
        }
    }

    char auto_save_dir[MAX_PATH];
    snprintf(auto_save_dir, sizeof(auto_save_dir), "%s\\TinyRequest\\auto_save", appdata_path);

    DWORD attrs = GetFileAttributesA(auto_save_dir);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryA(auto_save_dir, NULL)) {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS) {
                return -1;
            }
        }
    } else if (!(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return -1;
    }

    return 0;
#else
    char* home = getenv("HOME");
    if (!home) {
        return -1;
    }

    char auto_save_dir[1024];
    snprintf(auto_save_dir, sizeof(auto_save_dir), "%s/.config/tinyrequest/auto_save", home);

    if (mkdir(auto_save_dir, 0755) != 0) {
        struct stat st;
        if (stat(auto_save_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
            return -1;
        }
    }

    return 0;
#endif
}

char* persistence_get_collections_path(const char* filename) {
    if (!filename) {
        return NULL;
    }

#ifdef _WIN32
    char* appdata_path = getenv("LOCALAPPDATA");
    if (!appdata_path) {
        appdata_path = getenv("APPDATA");
        if (!appdata_path) {
            return NULL;
        }
    }

    char* full_path = (char*)malloc(MAX_PATH);
    if (!full_path) {
        return NULL;
    }

    snprintf(full_path, MAX_PATH, "%s\\TinyRequest\\collections\\%s", appdata_path, filename);
    return full_path;
#else
    char* home = getenv("HOME");
    if (!home) {
        return NULL;
    }

    char* full_path = (char*)malloc(1024);
    if (!full_path) {
        return NULL;
    }

    snprintf(full_path, 1024, "%s/.config/tinyrequest/collections/%s", home, filename);
    return full_path;
#endif
}

char* persistence_get_auto_save_path(const char* filename) {
    if (!filename) {
        return NULL;
    }

#ifdef _WIN32
    char* appdata_path = getenv("LOCALAPPDATA");
    if (!appdata_path) {
        appdata_path = getenv("APPDATA");
        if (!appdata_path) {
            return NULL;
        }
    }

    char* full_path = (char*)malloc(MAX_PATH);
    if (!full_path) {
        return NULL;
    }

    snprintf(full_path, MAX_PATH, "%s\\TinyRequest\\auto_save\\%s", appdata_path, filename);
    return full_path;
#else
    char* home = getenv("HOME");
    if (!home) {
        return NULL;
    }

    char* full_path = (char*)malloc(1024);
    if (!full_path) {
        return NULL;
    }

    snprintf(full_path, 1024, "%s/.config/tinyrequest/auto_save/%s", home, filename);
    return full_path;
#endif
}

int persistence_get_file_size(const char* filepath) {
    if (!filepath) {
        return -1;
    }

    FILE* file = fopen(filepath, "rb");
    if (!file) {
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);

    return (int)size;
}

time_t persistence_get_file_modified_time(const char* filepath) {
    if (!filepath) {
        return 0;
    }

#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (!GetFileAttributesExA(filepath, GetFileExInfoStandard, &attrs)) {
        return 0;
    }

    FILETIME ft = attrs.ftLastWriteTime;
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;

    return (time_t)((ull.QuadPart - 116444736000000000ULL) / 10000000ULL);
#else
    struct stat st;
    if (stat(filepath, &st) != 0) {
        return 0;
    }

    return st.st_mtime;
#endif
}

int persistence_validate_collection_file(const char* filepath) {
    if (!filepath) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    if (!persistence_file_exists(filepath)) {
        return PERSISTENCE_ERROR_FILE_NOT_FOUND;
    }

    FILE* file = fopen(filepath, "rb");
    if (!file) {
        return PERSISTENCE_ERROR_PERMISSION_DENIED;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        return PERSISTENCE_ERROR_CORRUPTED_DATA;
    }

    char buffer[256];
    size_t read_size = fread(buffer, 1, sizeof(buffer) - 1, file);
    buffer[read_size] = '\0';
    fclose(file);

    char* trimmed = buffer;
    while (*trimmed && (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n' || *trimmed == '\r')) {
        trimmed++;
    }

    if (*trimmed != '{' && *trimmed != '[') {
        return PERSISTENCE_ERROR_INVALID_JSON;
    }

    return PERSISTENCE_SUCCESS;
}

int persistence_handle_corrupted_file(const char* filepath, const char* operation) {
    if (!filepath || !operation) {
        return PERSISTENCE_ERROR_NULL_PARAM;
    }

    printf("Error: %s\n", persistence_get_user_friendly_error(PERSISTENCE_ERROR_CORRUPTED_DATA, operation));

    char backup_path[1024];
    snprintf(backup_path, sizeof(backup_path), "%s.corrupted.backup", filepath);

    FILE* source = fopen(filepath, "rb");
    FILE* dest = fopen(backup_path, "wb");

    if (source && dest) {
        char buffer[4096];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
            fwrite(buffer, 1, bytes, dest);
        }
        printf("Corrupted file backed up to: %s\n", backup_path);
    }

    if (source) fclose(source);
    if (dest) fclose(dest);

    return PERSISTENCE_ERROR_CORRUPTED_DATA;
}

const char* persistence_error_string(PersistenceError error) {
    switch (error) {
        case PERSISTENCE_SUCCESS:
            return "Success";
        case PERSISTENCE_ERROR_NULL_PARAM:
            return "Internal error: Invalid parameters";
        case PERSISTENCE_ERROR_FILE_NOT_FOUND:
            return "File not found or does not exist";
        case PERSISTENCE_ERROR_PERMISSION_DENIED:
            return "Permission denied - check file/folder permissions";
        case PERSISTENCE_ERROR_INVALID_JSON:
            return "Invalid or corrupted data format";
        case PERSISTENCE_ERROR_MEMORY_ALLOCATION:
            return "Out of memory - try closing other applications";
        case PERSISTENCE_ERROR_CORRUPTED_DATA:
            return "Data file is corrupted or invalid";
        case PERSISTENCE_ERROR_DISK_FULL:
            return "Disk full or unable to write file";
        case PERSISTENCE_ERROR_INVALID_PATH:
            return "Invalid file path or location";
        default:
            return "Unknown error occurred";
    }
}

const char* persistence_get_user_friendly_error(PersistenceError error, const char* operation) {
    static char error_buffer[512];

    switch (error) {
        case PERSISTENCE_SUCCESS:
            snprintf(error_buffer, sizeof(error_buffer), "%s completed successfully", operation);
            break;
        case PERSISTENCE_ERROR_FILE_NOT_FOUND:
            snprintf(error_buffer, sizeof(error_buffer), 
                    "Could not find the file for %s. The file may have been moved or deleted.", operation);
            break;
        case PERSISTENCE_ERROR_PERMISSION_DENIED:
            snprintf(error_buffer, sizeof(error_buffer), 
                    "Permission denied while trying to %s. Please check that you have write access to the folder.", operation);
            break;
        case PERSISTENCE_ERROR_INVALID_JSON:
            snprintf(error_buffer, sizeof(error_buffer), 
                    "The data file is corrupted or in an invalid format. Unable to %s.", operation);
            break;
        case PERSISTENCE_ERROR_MEMORY_ALLOCATION:
            snprintf(error_buffer, sizeof(error_buffer), 
                    "Out of memory while trying to %s. Try closing other applications and try again.", operation);
            break;
        case PERSISTENCE_ERROR_CORRUPTED_DATA:
            snprintf(error_buffer, sizeof(error_buffer), 
                    "The data file appears to be corrupted. Unable to %s.", operation);
            break;
        case PERSISTENCE_ERROR_DISK_FULL:
            snprintf(error_buffer, sizeof(error_buffer), 
                    "Not enough disk space to %s. Please free up some space and try again.", operation);
            break;
        case PERSISTENCE_ERROR_INVALID_PATH:
            snprintf(error_buffer, sizeof(error_buffer), 
                    "Invalid file location for %s. Please check the file path.", operation);
            break;
        default:
            snprintf(error_buffer, sizeof(error_buffer), 
                    "An unknown error occurred while trying to %s.", operation);
            break;
    }

    return error_buffer;
}