#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>
#include <time.h>
#include "request_response.h"
#include "http_client.h"
#include "collections.h"

#ifdef __cplusplus
extern "C" {
#endif

// Main tab enumeration for the three-tab interface
typedef enum {
    TAB_COLLECTIONS = 0,
    TAB_REQUEST = 1,
    TAB_RESPONSE = 2
} MainTab;

// Content type enumeration for buffer management
typedef enum {
    CONTENT_TYPE_JSON = 0,
    CONTENT_TYPE_FORM_DATA = 1,
    CONTENT_TYPE_FORM_URL_ENCODED = 2,
    CONTENT_TYPE_PLAIN_TEXT = 3,
    CONTENT_TYPE_XML = 4,
    CONTENT_TYPE_YAML = 5
} ContentType;

typedef struct {
    // Core functionality
    Request current_request;
    Response current_response;
    HttpClient* http_client;
    bool request_in_progress;
    char status_message[256];
    bool ssl_verify_enabled;
    
    // Collections system
    CollectionManager* collection_manager;
    
    // UI state for tab management
    MainTab active_tab;
    MainTab previous_tab;
    
    // Collection selection state
    int selected_collection_index;
    int selected_request_index;
    
    // UI state flags
    bool unsaved_changes;
    bool show_collection_create_dialog;
    bool show_collection_rename_dialog;
    bool show_request_create_dialog;
    bool show_cookie_manager;
    
    // UI input buffers (moved from UIManager - single source of truth)
    char collection_name_buffer[256];
    char collection_description_buffer[512];
    char request_name_buffer[256];
    char url_buffer[2048];
    char body_buffer[8192];  // Legacy buffer - kept for compatibility
    char header_name_buffer[128];
    char header_value_buffer[512];
    
    // Separate content type buffers to prevent cross-contamination
    char json_body_buffer[8192];
    char plain_text_body_buffer[8192];
    char xml_body_buffer[8192];
    char yaml_body_buffer[8192];
    // Form data is handled separately with key-value pairs, not stored in buffers
    
    // Authentication buffers
    int selected_auth_type;           // 0=None, 1=API Key, 2=Bearer Token, 3=Basic Auth, 4=OAuth 2.0
    char auth_api_key_name[128];      // API Key name/header name
    char auth_api_key_value[512];     // API Key value
    char auth_bearer_token[512];      // Bearer token value
    char auth_basic_username[256];    // Basic auth username
    char auth_basic_password[256];    // Basic auth password
    char auth_oauth_token[512];       // OAuth 2.0 access token
    int auth_api_key_location;        // 0=Header, 1=Query Parameter
    
    // Authentication enable/disable checkboxes
    bool auth_api_key_enabled;        // Whether to send API Key auth
    bool auth_bearer_enabled;         // Whether to send Bearer Token auth
    bool auth_basic_enabled;          // Whether to send Basic Auth
    bool auth_oauth_enabled;          // Whether to send OAuth 2.0 auth
    
    // UI presentation state
    int selected_method_index;
    bool show_headers_panel;
    bool show_body_panel;
    
    // Dialog state (moved from UIManager)
    bool show_save_dialog;
    char save_request_name[256];
    char save_error_message[512];
    bool show_load_dialog;
    char load_error_message[512];
    int selected_request_index_for_load;
    
    // Change tracking for state synchronization
    bool ui_state_dirty;
    bool request_data_dirty;
    time_t last_ui_sync;
    
    // Enhanced unsaved changes tracking
    time_t last_change_time;
    bool changes_since_last_save;
    
    // Application settings
    bool auto_save_enabled;
    int auto_save_interval;         // seconds
    time_t last_auto_save;
    
    // Import/Export state
    char last_export_path[1024];
    char last_import_path[1024];
    bool show_import_dialog;
    bool show_export_dialog;
} AppState;

// Application state management functions
AppState* app_state_create(void);
void app_state_destroy(AppState* state);
void app_state_reset_request(AppState* state);
void app_state_reset_response(AppState* state);

// Collections integration functions
Collection* app_state_get_active_collection(AppState* state);
Request* app_state_get_active_request(AppState* state);
int app_state_set_active_collection(AppState* state, int collection_index);
int app_state_set_active_request(AppState* state, int request_index);

// Tab management functions
void app_state_set_active_tab(AppState* state, MainTab tab);
MainTab app_state_get_active_tab(AppState* state);
MainTab app_state_get_previous_tab(AppState* state);

// UI state management functions
void app_state_set_unsaved_changes(AppState* state, bool has_changes);
bool app_state_has_unsaved_changes(AppState* state);
void app_state_clear_ui_buffers(AppState* state);
void app_state_clear_request_ui_buffers(AppState* state);

// Enhanced change tracking functions
void app_state_mark_changed(AppState* state);
void app_state_mark_saved(AppState* state);
bool app_state_has_changes_since_save(AppState* state);
time_t app_state_get_last_change_time(AppState* state);

// State synchronization functions
void app_state_sync_ui_to_request(AppState* state);
void app_state_sync_request_to_ui(AppState* state);
void app_state_mark_ui_dirty(AppState* state);
void app_state_mark_request_dirty(AppState* state);
bool app_state_needs_ui_sync(AppState* state);
bool app_state_needs_request_sync(AppState* state);
void app_state_auto_sync(AppState* state);

// Auto-save functions
bool app_state_should_auto_save(AppState* state);
void app_state_update_auto_save_time(AppState* state);
int app_state_perform_auto_save(AppState* state);
int app_state_save_all_collections(AppState* state);
void app_state_check_and_perform_auto_save(AppState* state);

// Content type buffer management functions
char* app_state_get_content_buffer(AppState* state, int content_type);
void app_state_set_content_buffer(AppState* state, int content_type, const char* content);
void app_state_clear_content_buffers(AppState* state);
void app_state_sync_content_to_body_buffer(AppState* state, int content_type);


#ifdef __cplusplus
}
#endif

#endif // APP_STATE_H