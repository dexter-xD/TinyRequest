/**
 * persistence.h
 * 
 * saving and loading data to disk for tinyrequest
 * 
 * this file handles all the file operations for keeping your requests
 * and collections safe between sessions. it can save individual requests,
 * entire collections, and all your application settings to json files.
 * 
 * includes auto-save functionality so you don't lose work, migration
 * tools for upgrading from older file formats, and error handling for
 * when disk operations go wrong. also handles creating the necessary
 * config directories and managing file paths.
 * 
 * basically everything you need to make sure your data survives crashes,
 * updates, and the occasional accidental quit.
 */

#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "request_response.h"
#include "collections.h"

#ifdef __cplusplus
extern "C" {
#endif

int persistence_save_request(const Request* request, const char* name, const char* filename);
int persistence_load_request(Request* request, const char* filename);

int persistence_save_collection_new(const Collection* collection, const char* filepath);
int persistence_save_collection_with_auth(const Collection* collection, const char* filepath, const void* app_state);
int persistence_load_collection_new(Collection* collection, const char* filepath);
int persistence_load_collection_with_auth(Collection* collection, const char* filepath, void* app_state);
int persistence_export_collection(const Collection* collection, const char* filepath);
int persistence_import_collection(Collection* collection, const char* filepath);

int persistence_save_all_collections(const CollectionManager* manager);
int persistence_save_all_collections_with_auth(const CollectionManager* manager, const void* app_state);
int persistence_load_all_collections(CollectionManager* manager);
int persistence_load_all_collections_with_auth(CollectionManager* manager, void* app_state);
int persistence_save_collection_manager_state(const CollectionManager* manager);
int persistence_load_collection_manager_state(CollectionManager* manager);
int persistence_delete_collection_file(const char* collection_id);

int persistence_auto_save_collections(const CollectionManager* manager);
int persistence_restore_from_auto_save(CollectionManager* manager);
int persistence_create_auto_save_backup(const CollectionManager* manager);
int persistence_cleanup_old_auto_saves(int keep_count);

int persistence_migrate_legacy_requests(CollectionManager* manager);
int persistence_load_legacy_and_create_collection(CollectionManager* manager, const char* legacy_path);
int persistence_create_default_collection_from_legacy(CollectionManager* manager, const void* legacy_collection);
int persistence_backup_legacy_data(void);

int persistence_save_settings(const CollectionManager* manager, bool auto_save_enabled, int auto_save_interval);
int persistence_load_settings(bool* auto_save_enabled, int* auto_save_interval);

int persistence_create_config_dir(void);
int persistence_create_collections_dir(void);
int persistence_create_auto_save_dir(void);
char* persistence_get_config_path(const char* filename);
char* persistence_get_collections_path(const char* filename);
char* persistence_get_auto_save_path(const char* filename);
int persistence_file_exists(const char* filepath);
int persistence_get_file_size(const char* filepath);
time_t persistence_get_file_modified_time(const char* filepath);

int persistence_validate_collection_file(const char* filepath);
int persistence_handle_corrupted_file(const char* filepath, const char* operation);

typedef enum {
    PERSISTENCE_SUCCESS = 0,
    PERSISTENCE_ERROR_NULL_PARAM = -1,
    PERSISTENCE_ERROR_FILE_NOT_FOUND = -2,
    PERSISTENCE_ERROR_PERMISSION_DENIED = -3,
    PERSISTENCE_ERROR_INVALID_JSON = -4,
    PERSISTENCE_ERROR_MEMORY_ALLOCATION = -5,
    PERSISTENCE_ERROR_CORRUPTED_DATA = -6,
    PERSISTENCE_ERROR_DISK_FULL = -7,
    PERSISTENCE_ERROR_INVALID_PATH = -8
} PersistenceError;

const char* persistence_error_string(PersistenceError error);
const char* persistence_get_user_friendly_error(PersistenceError error, const char* operation);

#ifdef __cplusplus
}
#endif

#endif