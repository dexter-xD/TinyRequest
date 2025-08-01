/**
 * central application state management for tinyrequest
 *
 * this file handles the main application state including request data,
 * collection management, ui synchronization, and persistence operations.
 * it acts as the bridge between the ui layer and the data layer, ensuring
 * that changes in one are properly reflected in the other.
 *
 * the state manager handles complex scenarios like switching between
 * collections and requests while preserving unsaved changes, managing
 * authentication data across different contexts, and coordinating
 * auto-save operations to prevent data loss.
 */

#include "app_state.h"
#include "persistence.h"
#include "collections.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* creates and initializes a new application state instance */
AppState* app_state_create(void) {
    AppState* state = (AppState*)malloc(sizeof(AppState));
    if (!state) {
        return NULL;
    }

    memset(state, 0, sizeof(AppState));

    request_init(&state->current_request);
    response_init(&state->current_response);

    state->collection_manager = collection_manager_create();
    if (!state->collection_manager) {
        request_cleanup(&state->current_request);
        response_cleanup(&state->current_response);
        free(state);
        return NULL;
    }

    state->selected_auth_type = 0;
    memset(state->auth_api_key_name, 0, sizeof(state->auth_api_key_name));
    memset(state->auth_api_key_value, 0, sizeof(state->auth_api_key_value));
    memset(state->auth_bearer_token, 0, sizeof(state->auth_bearer_token));
    memset(state->auth_basic_username, 0, sizeof(state->auth_basic_username));
    memset(state->auth_basic_password, 0, sizeof(state->auth_basic_password));
    memset(state->auth_oauth_token, 0, sizeof(state->auth_oauth_token));
    state->auth_api_key_location = 0;
    
    // Initialize authentication checkboxes (enabled by default for backward compatibility)
    state->auth_api_key_enabled = true;
    state->auth_bearer_enabled = true;
    state->auth_basic_enabled = true;
    state->auth_oauth_enabled = true;

    persistence_load_settings(&state->auto_save_enabled, &state->auto_save_interval);

    persistence_migrate_legacy_requests(state->collection_manager);
    
    persistence_load_all_collections_with_auth(state->collection_manager, state);

    state->http_client = http_client_create();
    if (!state->http_client) {
        collection_manager_destroy(state->collection_manager);
        request_cleanup(&state->current_request);
        response_cleanup(&state->current_response);
        free(state);
        return NULL;
    }

    state->active_tab = TAB_COLLECTIONS;
    state->previous_tab = TAB_COLLECTIONS;
    state->selected_collection_index = -1;
    state->selected_request_index = -1;
    state->unsaved_changes = false;

    state->show_collection_create_dialog = false;
    state->show_collection_rename_dialog = false;
    state->show_request_create_dialog = false;
    state->show_cookie_manager = false;
    state->show_import_dialog = false;
    state->show_export_dialog = false;

    memset(state->collection_name_buffer, 0, sizeof(state->collection_name_buffer));
    memset(state->collection_description_buffer, 0, sizeof(state->collection_description_buffer));
    memset(state->request_name_buffer, 0, sizeof(state->request_name_buffer));
    strncpy(state->url_buffer, "https://", sizeof(state->url_buffer) - 1);
    state->url_buffer[sizeof(state->url_buffer) - 1] = '\0';
    memset(state->body_buffer, 0, sizeof(state->body_buffer));
    memset(state->header_name_buffer, 0, sizeof(state->header_name_buffer));
    memset(state->header_value_buffer, 0, sizeof(state->header_value_buffer));
    memset(state->last_export_path, 0, sizeof(state->last_export_path));
    memset(state->last_import_path, 0, sizeof(state->last_import_path));

    memset(state->json_body_buffer, 0, sizeof(state->json_body_buffer));
    memset(state->plain_text_body_buffer, 0, sizeof(state->plain_text_body_buffer));
    memset(state->xml_body_buffer, 0, sizeof(state->xml_body_buffer));
    memset(state->yaml_body_buffer, 0, sizeof(state->yaml_body_buffer));

    state->selected_method_index = 0; 
    state->show_headers_panel = true;
    state->show_body_panel = true;

    state->show_save_dialog = false;
    memset(state->save_request_name, 0, sizeof(state->save_request_name));
    memset(state->save_error_message, 0, sizeof(state->save_error_message));
    state->show_load_dialog = false;
    memset(state->load_error_message, 0, sizeof(state->load_error_message));
    state->selected_request_index_for_load = -1;

    state->ui_state_dirty = false;
    state->request_data_dirty = false;
    state->last_ui_sync = time(NULL);
    state->last_change_time = time(NULL);
    state->changes_since_last_save = false;

    state->auto_save_enabled = true;
    state->auto_save_interval = 300; 
    state->last_auto_save = time(NULL);

    strncpy(state->status_message, "Ready", sizeof(state->status_message) - 1);
    state->status_message[sizeof(state->status_message) - 1] = '\0';
    state->request_in_progress = false;
    state->ssl_verify_enabled = true; 

    if (state->collection_manager->count > 0) {

        state->selected_collection_index = 0;
        collection_manager_set_active_collection(state->collection_manager, 0);

        Collection* first_collection = &state->collection_manager->collections[0];
        if (first_collection->request_count > 0) {
            state->selected_request_index = 0;
            collection_manager_set_active_request(state->collection_manager, 0);

            state->request_data_dirty = true;

            Request* active_request = app_state_get_active_request(state);
            if (active_request) {

                const char* methods[] = { "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS" };
                const int method_count = sizeof(methods) / sizeof(methods[0]);

                int new_method_index = 0; 
                for (int i = 0; i < method_count; i++) {
                    if (strcmp(active_request->method, methods[i]) == 0) {
                        new_method_index = i;
                        break;
                    }
                }
                state->selected_method_index = new_method_index;

                strncpy(state->url_buffer, active_request->url, sizeof(state->url_buffer) - 1);
                state->url_buffer[sizeof(state->url_buffer) - 1] = '\0';

                if (active_request->body && active_request->body_size > 0) {
                    size_t copy_size = (active_request->body_size < sizeof(state->body_buffer) - 1) ?
                                      active_request->body_size : sizeof(state->body_buffer) - 1;
                    memcpy(state->body_buffer, active_request->body, copy_size);
                    state->body_buffer[copy_size] = '\0';

                    const char* content = state->body_buffer;

                    app_state_clear_content_buffers(state);

                    const char* content_type_header = NULL;
                    for (int i = 0; i < active_request->headers.count; i++) {
                        if (strcasecmp(active_request->headers.headers[i].name, "content-type") == 0) {
                            content_type_header = active_request->headers.headers[i].value;
                            break;
                        }
                    }

                    bool content_distributed = false;

                    if (content_type_header) {
                        if (strstr(content_type_header, "application/json") != NULL) {
                            app_state_set_content_buffer(state, CONTENT_TYPE_JSON, content);
                            content_distributed = true;
                        } else if (strstr(content_type_header, "multipart/form-data") != NULL) {
                            content_distributed = true; 
                        } else if (strstr(content_type_header, "application/x-www-form-urlencoded") != NULL) {
                            content_distributed = true; 
                        } else if (strstr(content_type_header, "text/plain") != NULL) {
                            app_state_set_content_buffer(state, CONTENT_TYPE_PLAIN_TEXT, content);
                            content_distributed = true;
                        } else if (strstr(content_type_header, "application/xml") != NULL || strstr(content_type_header, "text/xml") != NULL) {
                            app_state_set_content_buffer(state, CONTENT_TYPE_XML, content);
                            content_distributed = true;
                        } else if (strstr(content_type_header, "application/x-yaml") != NULL || strstr(content_type_header, "text/yaml") != NULL) {
                            app_state_set_content_buffer(state, CONTENT_TYPE_YAML, content);
                            content_distributed = true;
                        }
                    }

                    if (!content_distributed && strlen(content) > 0) {
                        const char* trimmed = content;
                        while (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n' || *trimmed == '\r') {
                            trimmed++;
                        }

                        if (*trimmed == '{' || *trimmed == '[') {
                            app_state_set_content_buffer(state, CONTENT_TYPE_JSON, content);
                        } else if (strstr(content, "--TinyRequestFormBoundary") != NULL) {

                        } else if (strstr(content, "=") != NULL && strstr(content, "&") != NULL) {

                        } else if (*trimmed == '<' && strstr(content, ">") != NULL) {
                            app_state_set_content_buffer(state, CONTENT_TYPE_XML, content);
                        } else if (strstr(content, ":") != NULL && (strstr(content, "\n") != NULL || strstr(content, "\r") != NULL)) {
                            app_state_set_content_buffer(state, CONTENT_TYPE_YAML, content);
                        } else {
                            app_state_set_content_buffer(state, CONTENT_TYPE_PLAIN_TEXT, content);
                        }
                    }
                } else {
                    state->body_buffer[0] = '\0';
                    app_state_clear_content_buffers(state);
                }

                if (active_request) {
                    // Load authentication data from the active request into global state for UI display
                    // This follows the same pattern as URL and body loading above
                    state->selected_auth_type = active_request->selected_auth_type;
                    strncpy(state->auth_api_key_name, active_request->auth_api_key_name, sizeof(state->auth_api_key_name) - 1);
                    state->auth_api_key_name[sizeof(state->auth_api_key_name) - 1] = '\0';
                    strncpy(state->auth_api_key_value, active_request->auth_api_key_value, sizeof(state->auth_api_key_value) - 1);
                    state->auth_api_key_value[sizeof(state->auth_api_key_value) - 1] = '\0';
                    strncpy(state->auth_bearer_token, active_request->auth_bearer_token, sizeof(state->auth_bearer_token) - 1);
                    state->auth_bearer_token[sizeof(state->auth_bearer_token) - 1] = '\0';
                    strncpy(state->auth_basic_username, active_request->auth_basic_username, sizeof(state->auth_basic_username) - 1);
                    state->auth_basic_username[sizeof(state->auth_basic_username) - 1] = '\0';
                    strncpy(state->auth_basic_password, active_request->auth_basic_password, sizeof(state->auth_basic_password) - 1);
                    state->auth_basic_password[sizeof(state->auth_basic_password) - 1] = '\0';
                    strncpy(state->auth_oauth_token, active_request->auth_oauth_token, sizeof(state->auth_oauth_token) - 1);
                    state->auth_oauth_token[sizeof(state->auth_oauth_token) - 1] = '\0';
                    state->auth_api_key_location = active_request->auth_api_key_location;
                    
                    // Load authentication checkboxes
                    state->auth_api_key_enabled = active_request->auth_api_key_enabled;
                    state->auth_bearer_enabled = active_request->auth_bearer_enabled;
                    state->auth_basic_enabled = active_request->auth_basic_enabled;
                    state->auth_oauth_enabled = active_request->auth_oauth_enabled;
                }

                state->request_data_dirty = false;
                state->last_ui_sync = time(NULL);
            }
        }
    }

    return state;
}

/* safely destroys an application state instance and frees all resources */
void app_state_destroy(AppState* state) {
    if (!state) {
        return;
    }

    if (state->http_client) {
        http_client_destroy(state->http_client);
        state->http_client = NULL;
    }

    if (state->collection_manager) {
        collection_manager_destroy(state->collection_manager);
        state->collection_manager = NULL;
    }

    request_cleanup(&state->current_request);
    response_cleanup(&state->current_response);

    free(state);
}

/* resets the current request to its initial state */
void app_state_reset_request(AppState* state) {
    if (!state) {
        return;
    }

    request_cleanup(&state->current_request);
    request_init(&state->current_request);
}

/* resets the current response to its initial state */
void app_state_reset_response(AppState* state) {
    if (!state) {
        return;
    }

    response_cleanup(&state->current_response);
    response_init(&state->current_response);
}

/* returns the currently active collection or null if none selected */
Collection* app_state_get_active_collection(AppState* state) {
    if (!state || !state->collection_manager) {
        return NULL;
    }

    return collection_manager_get_active_collection(state->collection_manager);
}

/* returns the currently active request or null if none selected */
Request* app_state_get_active_request(AppState* state) {
    if (!state || !state->collection_manager) {
        return NULL;
    }

    return collection_manager_get_active_request(state->collection_manager);
}

/* sets the active collection and handles data synchronization */
int app_state_set_active_collection(AppState* state, int collection_index) {
    if (!state || !state->collection_manager) {
        return -1;
    }

    if (state->ui_state_dirty) {
        app_state_sync_ui_to_request(state);
    }

    bool is_switching_collections = (state->collection_manager->active_collection_index != collection_index);

    int result = collection_manager_set_active_collection(state->collection_manager, collection_index);
    if (result == 0) {
        state->selected_collection_index = collection_index;

        state->selected_request_index = state->collection_manager->active_request_index;

        if (is_switching_collections) {

            Collection* new_collection = app_state_get_active_collection(state);
            if (new_collection) {
                char filename[128];
                snprintf(filename, sizeof(filename), "%s.json", new_collection->id);
                char* filepath = persistence_get_collections_path(filename);
                if (filepath) {

                    persistence_load_collection_with_auth(new_collection, filepath, state);
                    free(filepath);
                }
            }
        }

        if (state->selected_request_index >= 0) {
            Request* active_request = app_state_get_active_request(state);
            if (active_request) {

                app_state_set_active_request(state, state->selected_request_index);
            }
        } else {

            app_state_clear_request_ui_buffers(state);
        }
    }

    return result;
}

/* sets the active request within the current collection */
int app_state_set_active_request(AppState* state, int request_index) {
    if (!state || !state->collection_manager) {
        return -1;
    }

    if (state->ui_state_dirty) {
        app_state_sync_ui_to_request(state);
    }

    int result = collection_manager_set_active_request(state->collection_manager, request_index);
    if (result == 0) {
        state->selected_request_index = request_index;

        app_state_mark_request_dirty(state);
        // Immediately sync request data to UI to ensure authentication data loads
        app_state_sync_request_to_ui(state);
    }

    return result;
}

/* switches to a different main tab in the ui */
void app_state_set_active_tab(AppState* state, MainTab tab) {
    if (!state) {
        return;
    }

    state->previous_tab = state->active_tab;
    state->active_tab = tab;
}

/* returns the currently active tab */
MainTab app_state_get_active_tab(AppState* state) {
    if (!state) {
        return TAB_COLLECTIONS;
    }

    return state->active_tab;
}

/* returns the previously active tab */
MainTab app_state_get_previous_tab(AppState* state) {
    if (!state) {
        return TAB_COLLECTIONS;
    }

    return state->previous_tab;
}

/* marks the application as having unsaved changes */
void app_state_set_unsaved_changes(AppState* state, bool has_changes) {
    if (!state) {
        return;
    }

    if (has_changes) {
        app_state_mark_changed(state);
    } else {
        app_state_mark_saved(state);
    }
}

/* checks if there are any unsaved changes in the application */
bool app_state_has_unsaved_changes(AppState* state) {
    if (!state) {
        return false;
    }

    return state->unsaved_changes;
}

/* clears dialog-related ui buffers without affecting request data */
void app_state_clear_ui_buffers(AppState* state) {
    if (!state) {
        return;
    }

    memset(state->collection_name_buffer, 0, sizeof(state->collection_name_buffer));
    memset(state->collection_description_buffer, 0, sizeof(state->collection_description_buffer));
    memset(state->request_name_buffer, 0, sizeof(state->request_name_buffer));
}

/* clears request ui buffers while preserving authentication data */
void app_state_clear_request_ui_buffers(AppState* state) {
    if (!state) {
        return;
    }

    memset(state->collection_name_buffer, 0, sizeof(state->collection_name_buffer));
    memset(state->collection_description_buffer, 0, sizeof(state->collection_description_buffer));
    memset(state->request_name_buffer, 0, sizeof(state->request_name_buffer));

    strncpy(state->url_buffer, "https://", sizeof(state->url_buffer) - 1);  
    state->url_buffer[sizeof(state->url_buffer) - 1] = '\0';
    memset(state->body_buffer, 0, sizeof(state->body_buffer));
    memset(state->header_name_buffer, 0, sizeof(state->header_name_buffer));
    memset(state->header_value_buffer, 0, sizeof(state->header_value_buffer));

    app_state_clear_content_buffers(state);

    state->selected_method_index = 0;

    request_cleanup(&state->current_request);
    request_init(&state->current_request);
    strncpy(state->current_request.method, "GET", sizeof(state->current_request.method) - 1);
    state->current_request.method[sizeof(state->current_request.method) - 1] = '\0';
    strncpy(state->current_request.url, "https://", sizeof(state->current_request.url) - 1);
    state->current_request.url[sizeof(state->current_request.url) - 1] = '\0';

    app_state_mark_ui_dirty(state);
}

/* determines if an auto-save operation should be performed */
bool app_state_should_auto_save(AppState* state) {
    if (!state || !state->auto_save_enabled) {
        return false;
    }

    time_t current_time = time(NULL);
    return (current_time - state->last_auto_save) >= state->auto_save_interval;
}

/* updates the last auto-save timestamp */
void app_state_update_auto_save_time(AppState* state) {
    if (!state) {
        return;
    }

    state->last_auto_save = time(NULL);
}

/* performs an auto-save operation with authentication data */
int app_state_perform_auto_save(AppState* state) {
    if (!state || !state->collection_manager) {
        return -1;
    }

    int result = persistence_save_all_collections_with_auth(state->collection_manager, state);
    if (result == PERSISTENCE_SUCCESS) {
        app_state_update_auto_save_time(state);
        return 0;
    } else {
        return -1;
    }
}

/* saves all collections and settings to persistent storage */
int app_state_save_all_collections(AppState* state) {
    if (!state || !state->collection_manager) {
        return -1;
    }

    int result = persistence_save_all_collections_with_auth(state->collection_manager, state);
    if (result == PERSISTENCE_SUCCESS) {

        persistence_save_settings(state->collection_manager,
                                 state->auto_save_enabled,
                                 state->auto_save_interval);
        app_state_update_auto_save_time(state);
        return 0;
    } else {
        return -1;
    }
}

/* checks if auto-save is needed and performs it if necessary */
void app_state_check_and_perform_auto_save(AppState* state) {
    if (!state || !state->auto_save_enabled) {
        return;
    }

    if (app_state_should_auto_save(state)) {
        app_state_perform_auto_save(state);
    }
}

/* synchronizes ui buffer data to the active request structure */
void app_state_sync_ui_to_request(AppState* state) {
    if (!state) {
        return;
    }

    Request* active_request = app_state_get_active_request(state);
    if (!active_request) {
        active_request = &state->current_request;

        if (strlen(active_request->method) == 0) {
            strncpy(active_request->method, "GET", sizeof(active_request->method) - 1);
            active_request->method[sizeof(active_request->method) - 1] = '\0';
        }
        if (strlen(active_request->url) == 0) {
            strncpy(active_request->url, "https://", sizeof(active_request->url) - 1);
            active_request->url[sizeof(active_request->url) - 1] = '\0';
        }
    }

    bool changes_made = false;

    // Synchronize authentication data from global state to request
    if (active_request->selected_auth_type != state->selected_auth_type) {
        active_request->selected_auth_type = state->selected_auth_type;
        changes_made = true;
    }
    
    if (strcmp(active_request->auth_api_key_name, state->auth_api_key_name) != 0) {
        strncpy(active_request->auth_api_key_name, state->auth_api_key_name, sizeof(active_request->auth_api_key_name) - 1);
        active_request->auth_api_key_name[sizeof(active_request->auth_api_key_name) - 1] = '\0';
        changes_made = true;
    }
    
    if (strcmp(active_request->auth_api_key_value, state->auth_api_key_value) != 0) {
        strncpy(active_request->auth_api_key_value, state->auth_api_key_value, sizeof(active_request->auth_api_key_value) - 1);
        active_request->auth_api_key_value[sizeof(active_request->auth_api_key_value) - 1] = '\0';
        changes_made = true;
    }
    
    if (strcmp(active_request->auth_bearer_token, state->auth_bearer_token) != 0) {
        strncpy(active_request->auth_bearer_token, state->auth_bearer_token, sizeof(active_request->auth_bearer_token) - 1);
        active_request->auth_bearer_token[sizeof(active_request->auth_bearer_token) - 1] = '\0';
        changes_made = true;
    }
    
    if (strcmp(active_request->auth_basic_username, state->auth_basic_username) != 0) {
        strncpy(active_request->auth_basic_username, state->auth_basic_username, sizeof(active_request->auth_basic_username) - 1);
        active_request->auth_basic_username[sizeof(active_request->auth_basic_username) - 1] = '\0';
        changes_made = true;
    }
    
    if (strcmp(active_request->auth_basic_password, state->auth_basic_password) != 0) {
        strncpy(active_request->auth_basic_password, state->auth_basic_password, sizeof(active_request->auth_basic_password) - 1);
        active_request->auth_basic_password[sizeof(active_request->auth_basic_password) - 1] = '\0';
        changes_made = true;
    }
    
    if (strcmp(active_request->auth_oauth_token, state->auth_oauth_token) != 0) {
        strncpy(active_request->auth_oauth_token, state->auth_oauth_token, sizeof(active_request->auth_oauth_token) - 1);
        active_request->auth_oauth_token[sizeof(active_request->auth_oauth_token) - 1] = '\0';
        changes_made = true;
    }
    
    if (active_request->auth_api_key_location != state->auth_api_key_location) {
        active_request->auth_api_key_location = state->auth_api_key_location;
        changes_made = true;
    }
    
    // Synchronize authentication checkboxes
    if (active_request->auth_api_key_enabled != state->auth_api_key_enabled) {
        active_request->auth_api_key_enabled = state->auth_api_key_enabled;
        changes_made = true;
    }
    
    if (active_request->auth_bearer_enabled != state->auth_bearer_enabled) {
        active_request->auth_bearer_enabled = state->auth_bearer_enabled;
        changes_made = true;
    }
    
    if (active_request->auth_basic_enabled != state->auth_basic_enabled) {
        active_request->auth_basic_enabled = state->auth_basic_enabled;
        changes_made = true;
    }
    
    if (active_request->auth_oauth_enabled != state->auth_oauth_enabled) {
        active_request->auth_oauth_enabled = state->auth_oauth_enabled;
        changes_made = true;
    }

    const char* methods[] = { "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS" };
    const int method_count = sizeof(methods) / sizeof(methods[0]);
    if (state->selected_method_index >= 0 && state->selected_method_index < method_count) {
        const char* new_method = methods[state->selected_method_index];
        if (strcmp(active_request->method, new_method) != 0) {
            strncpy(active_request->method, new_method, sizeof(active_request->method) - 1);
            active_request->method[sizeof(active_request->method) - 1] = '\0';
            changes_made = true;
        }
    }

    if (strcmp(active_request->url, state->url_buffer) != 0) {
        strncpy(active_request->url, state->url_buffer, sizeof(active_request->url) - 1);
        active_request->url[sizeof(active_request->url) - 1] = '\0';
        changes_made = true;
    }

    const char* current_method = (state->selected_method_index >= 0 && state->selected_method_index < method_count) ?
                                methods[state->selected_method_index] : "GET";
    bool method_supports_body = (strcmp(current_method, "POST") == 0 ||
                                strcmp(current_method, "PUT") == 0 ||
                                strcmp(current_method, "PATCH") == 0 ||
                                strcmp(current_method, "DELETE") == 0);

    if (method_supports_body && strlen(state->body_buffer) > 0) {

        bool body_changed = false;
        size_t body_buffer_len = strlen(state->body_buffer);

        if (!active_request->body || active_request->body_size != body_buffer_len ||
            memcmp(active_request->body, state->body_buffer, body_buffer_len) != 0) {
            body_changed = true;
        }

        if (body_changed) {
            int result = request_set_body(active_request, state->body_buffer, body_buffer_len);
            if (result == 0) {
                changes_made = true;
            }
        }
    } else {

        if (active_request->body) {
            free(active_request->body);
            active_request->body = NULL;
            active_request->body_size = 0;
            changes_made = true;
        }
    }

    if (active_request != &state->current_request) {
        changes_made = true;
    }

    state->ui_state_dirty = false;
    state->last_ui_sync = time(NULL);

    if (changes_made && active_request != &state->current_request) {
        app_state_mark_changed(state);
    }
}

/* synchronizes active request data to ui buffers */
void app_state_sync_request_to_ui(AppState* state) {
    if (!state) {
        return;
    }

    Request* active_request = app_state_get_active_request(state);
    if (!active_request) {
        active_request = &state->current_request;
    }

    printf("DEBUG: app_state_sync_request_to_ui called - active_request: %p\n", (void*)active_request);
    if (active_request) {
        printf("DEBUG: BEFORE sync - request auth data - type: %d, api_key_name: '%s', bearer_token: '%s', api_key_enabled: %s, bearer_enabled: %s\n",
               active_request->selected_auth_type,
               active_request->auth_api_key_name,
               active_request->auth_bearer_token,
               active_request->auth_api_key_enabled ? "true" : "false",
               active_request->auth_bearer_enabled ? "true" : "false");
        printf("DEBUG: BEFORE sync - global state auth data - type: %d, api_key_name: '%s', bearer_token: '%s'\n",
               state->selected_auth_type,
               state->auth_api_key_name,
               state->auth_bearer_token);
    }

    const char* methods[] = { "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS" };
    const int method_count = sizeof(methods) / sizeof(methods[0]);

    int new_method_index = 0;
    for (int i = 0; i < method_count; i++) {
        if (strcmp(active_request->method, methods[i]) == 0) {
            new_method_index = i;
            break;
        }
    }
    state->selected_method_index = new_method_index;

    strncpy(state->url_buffer, active_request->url, sizeof(state->url_buffer) - 1);
    state->url_buffer[sizeof(state->url_buffer) - 1] = '\0';

    // Synchronize authentication data from request to global state for UI display
    // Note: This is for backward compatibility with existing UI code that reads from global state
    state->selected_auth_type = active_request->selected_auth_type;
    strncpy(state->auth_api_key_name, active_request->auth_api_key_name, sizeof(state->auth_api_key_name) - 1);
    state->auth_api_key_name[sizeof(state->auth_api_key_name) - 1] = '\0';
    strncpy(state->auth_api_key_value, active_request->auth_api_key_value, sizeof(state->auth_api_key_value) - 1);
    state->auth_api_key_value[sizeof(state->auth_api_key_value) - 1] = '\0';
    strncpy(state->auth_bearer_token, active_request->auth_bearer_token, sizeof(state->auth_bearer_token) - 1);
    state->auth_bearer_token[sizeof(state->auth_bearer_token) - 1] = '\0';
    strncpy(state->auth_basic_username, active_request->auth_basic_username, sizeof(state->auth_basic_username) - 1);
    state->auth_basic_username[sizeof(state->auth_basic_username) - 1] = '\0';
    strncpy(state->auth_basic_password, active_request->auth_basic_password, sizeof(state->auth_basic_password) - 1);
    state->auth_basic_password[sizeof(state->auth_basic_password) - 1] = '\0';
    strncpy(state->auth_oauth_token, active_request->auth_oauth_token, sizeof(state->auth_oauth_token) - 1);
    state->auth_oauth_token[sizeof(state->auth_oauth_token) - 1] = '\0';
    state->auth_api_key_location = active_request->auth_api_key_location;
    
    // Synchronize authentication checkboxes
    state->auth_api_key_enabled = active_request->auth_api_key_enabled;
    state->auth_bearer_enabled = active_request->auth_bearer_enabled;
    state->auth_basic_enabled = active_request->auth_basic_enabled;
    state->auth_oauth_enabled = active_request->auth_oauth_enabled;
    
    printf("DEBUG: AFTER sync - global state auth data - type: %d, api_key_name: '%s', bearer_token: '%s', api_key_enabled: %s, bearer_enabled: %s\n",
           state->selected_auth_type,
           state->auth_api_key_name,
           state->auth_bearer_token,
           state->auth_api_key_enabled ? "true" : "false",
           state->auth_bearer_enabled ? "true" : "false");

    if (active_request->body && active_request->body_size > 0) {
        size_t copy_size = (active_request->body_size < sizeof(state->body_buffer) - 1) ?
                          active_request->body_size : sizeof(state->body_buffer) - 1;
        memcpy(state->body_buffer, active_request->body, copy_size);
        state->body_buffer[copy_size] = '\0';

        const char* content = state->body_buffer;

        app_state_clear_content_buffers(state);

        const char* content_type_header = NULL;
        for (int i = 0; i < active_request->headers.count; i++) {
            if (strcasecmp(active_request->headers.headers[i].name, "content-type") == 0) {
                content_type_header = active_request->headers.headers[i].value;
                break;
            }
        }


        bool content_distributed = false;

        if (content_type_header) {
            if (strstr(content_type_header, "application/json") != NULL) {
                app_state_set_content_buffer(state, CONTENT_TYPE_JSON, content);
                content_distributed = true;
            } else if (strstr(content_type_header, "multipart/form-data") != NULL) {
                content_distributed = true;
            } else if (strstr(content_type_header, "application/x-www-form-urlencoded") != NULL) {
                content_distributed = true;
            } else if (strstr(content_type_header, "text/plain") != NULL) {
                app_state_set_content_buffer(state, CONTENT_TYPE_PLAIN_TEXT, content);
                content_distributed = true;
            } else if (strstr(content_type_header, "application/xml") != NULL || strstr(content_type_header, "text/xml") != NULL) {
                app_state_set_content_buffer(state, CONTENT_TYPE_XML, content);
                content_distributed = true;
            } else if (strstr(content_type_header, "application/x-yaml") != NULL || strstr(content_type_header, "text/yaml") != NULL) {
                app_state_set_content_buffer(state, CONTENT_TYPE_YAML, content);
                content_distributed = true;
            }
        }

        if (!content_distributed && strlen(content) > 0) {
            const char* trimmed = content;
            while (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n' || *trimmed == '\r') {
                trimmed++;
            }

            if (*trimmed == '{' || *trimmed == '[') {
                app_state_set_content_buffer(state, CONTENT_TYPE_JSON, content);
            } else if (strstr(content, "--TinyRequestFormBoundary") != NULL) {
                // Keep multipart form data in body_buffer
            } else if (strstr(content, "=") != NULL && strstr(content, "&") != NULL) {
                // Keep URL encoded form data in body_buffer
            } else if (*trimmed == '<' && strstr(content, ">") != NULL) {
                app_state_set_content_buffer(state, CONTENT_TYPE_XML, content);
            } else if (strstr(content, ":") != NULL && (strstr(content, "\n") != NULL || strstr(content, "\r") != NULL)) {
                app_state_set_content_buffer(state, CONTENT_TYPE_YAML, content);
            } else {
                app_state_set_content_buffer(state, CONTENT_TYPE_PLAIN_TEXT, content);
            }
        }
    } else {
        state->body_buffer[0] = '\0';
        app_state_clear_content_buffers(state);
    }

    state->request_data_dirty = false;
    state->last_ui_sync = time(NULL);
}

/* marks ui state as dirty to trigger synchronization */
void app_state_mark_ui_dirty(AppState* state) {
    if (state) {
        state->ui_state_dirty = true;
    }
}

/* marks request data as dirty to trigger ui updates */
void app_state_mark_request_dirty(AppState* state) {
    if (state) {
        state->request_data_dirty = true;
    }
}

/* checks if ui synchronization is needed */
bool app_state_needs_ui_sync(AppState* state) {
    return state ? state->ui_state_dirty : false;
}

/* checks if request synchronization is needed */
bool app_state_needs_request_sync(AppState* state) {
    return state ? state->request_data_dirty : false;
}

/* performs automatic synchronization between ui and request data */
void app_state_auto_sync(AppState* state) {
    if (!state) {
        return;
    }

    if (state->request_data_dirty) {
        app_state_sync_request_to_ui(state);
    }

    if (state->ui_state_dirty && !state->show_request_create_dialog && !state->show_collection_create_dialog) {
        app_state_sync_ui_to_request(state);
    }
}

/* marks the application state as having unsaved changes */
void app_state_mark_changed(AppState* state) {
    if (!state) {
        return;
    }

    state->unsaved_changes = true;
    state->changes_since_last_save = true;
    state->last_change_time = time(NULL);
}

/* marks the application state as saved */
void app_state_mark_saved(AppState* state) {
    if (!state) {
        return;
    }

    state->unsaved_changes = false;
    state->changes_since_last_save = false;

}

/* checks if there are changes since the last save operation */
bool app_state_has_changes_since_save(AppState* state) {
    return state ? state->changes_since_last_save : false;
}

/* returns the timestamp of the last change */
time_t app_state_get_last_change_time(AppState* state) {
    return state ? state->last_change_time : 0;
}

/* returns the appropriate content buffer for the specified type */
char* app_state_get_content_buffer(AppState* state, int content_type) {
    if (!state) {
        return NULL;
    }

    switch (content_type) {
        case CONTENT_TYPE_JSON:
            return state->json_body_buffer;
        case CONTENT_TYPE_PLAIN_TEXT:
            return state->plain_text_body_buffer;
        case CONTENT_TYPE_XML:
            return state->xml_body_buffer;
        case CONTENT_TYPE_YAML:
            return state->yaml_body_buffer;
        case CONTENT_TYPE_FORM_DATA:
        case CONTENT_TYPE_FORM_URL_ENCODED:

            return state->body_buffer;
        default:
            return state->body_buffer; 
    }
}

/* sets content in the appropriate buffer based on content type */
void app_state_set_content_buffer(AppState* state, int content_type, const char* content) {
    if (!state || !content) {
        return;
    }

    char* buffer = app_state_get_content_buffer(state, content_type);
    if (buffer) {
        size_t buffer_size;
        switch (content_type) {
            case CONTENT_TYPE_JSON:
                buffer_size = sizeof(state->json_body_buffer);
                break;
            case CONTENT_TYPE_PLAIN_TEXT:
                buffer_size = sizeof(state->plain_text_body_buffer);
                break;
            case CONTENT_TYPE_XML:
                buffer_size = sizeof(state->xml_body_buffer);
                break;
            case CONTENT_TYPE_YAML:
                buffer_size = sizeof(state->yaml_body_buffer);
                break;
            default:
                buffer_size = sizeof(state->body_buffer);
                break;
        }

        strncpy(buffer, content, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        app_state_mark_ui_dirty(state);
        app_state_set_unsaved_changes(state, true);
    }
}

/* clears all content type specific buffers */
void app_state_clear_content_buffers(AppState* state) {
    if (!state) {
        return;
    }

    memset(state->json_body_buffer, 0, sizeof(state->json_body_buffer));
    memset(state->plain_text_body_buffer, 0, sizeof(state->plain_text_body_buffer));
    memset(state->xml_body_buffer, 0, sizeof(state->xml_body_buffer));
    memset(state->yaml_body_buffer, 0, sizeof(state->yaml_body_buffer));
}

/* synchronizes content from specific buffer to main body buffer */
void app_state_sync_content_to_body_buffer(AppState* state, int content_type) {
    if (!state) {
        return;
    }

    char* source_buffer = app_state_get_content_buffer(state, content_type);
    if (source_buffer) {

        if (source_buffer != state->body_buffer) {
            strncpy(state->body_buffer, source_buffer, sizeof(state->body_buffer) - 1);
            state->body_buffer[sizeof(state->body_buffer) - 1] = '\0';
        }
    }
}