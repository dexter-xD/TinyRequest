/**
 * ui_main_tabs.h
 * 
 * main tab interface and collections management for tinyrequest
 * 
 * this file handles the main tabbed interface that lets users switch
 * between collections, request editing, and response viewing. it manages
 * the collections tree view where users can organize their requests
 * into folders and provides all the dialogs for creating, renaming,
 * and managing collections and requests.
 * 
 * includes context menus for right-clicking on collections and requests,
 * drag-and-drop support for moving requests between collections, and
 * all the styling to make the interface look modern and professional.
 * 
 * basically the main workspace where users spend most of their time
 * organizing and working with their http requests.
 */

#ifndef UI_MAIN_TABS_H
#define UI_MAIN_TABS_H

#include <stdbool.h>
#include "ui_manager.h"
#include "app_state.h"
#include "ui/theme.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_main_tabs_render(UIManager* ui, AppState* state);

void ui_main_tabs_render_collections_tab(UIManager* ui, AppState* state);
void ui_main_tabs_render_request_tab(UIManager* ui, AppState* state);
void ui_main_tabs_render_response_tab(UIManager* ui, AppState* state);

void ui_main_tabs_handle_tab_switch(UIManager* ui, AppState* state, MainTab new_tab);
bool ui_main_tabs_should_show_tab(AppState* state, MainTab tab);

void ui_main_tabs_push_tab_style(bool is_active);
void ui_main_tabs_pop_tab_style(void);

void ui_collections_render_empty_state(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme);
void ui_collections_render_tree_view(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme);
void ui_collections_render_collection_node(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme, int collection_index);
void ui_collections_render_request_node(UIManager* ui, AppState* state, const ModernGruvboxTheme* theme, int collection_index, int request_index);

void ui_collections_render_create_dialog(UIManager* ui, AppState* state);
void ui_collections_render_rename_dialog(UIManager* ui, AppState* state);
void ui_collections_render_request_create_dialog(UIManager* ui, AppState* state);

void ui_collections_render_collection_context_menu(UIManager* ui, AppState* state, int collection_index);
void ui_collections_render_request_context_menu(UIManager* ui, AppState* state, int collection_index, int request_index);

bool ui_collections_handle_create_collection(UIManager* ui, AppState* state);
bool ui_collections_handle_rename_collection(UIManager* ui, AppState* state, int collection_index);
bool ui_collections_handle_delete_collection(UIManager* ui, AppState* state, int collection_index);
bool ui_collections_handle_create_request(UIManager* ui, AppState* state, int collection_index);
bool ui_collections_handle_delete_request(UIManager* ui, AppState* state, int collection_index, int request_index);
bool ui_collections_handle_duplicate_request(UIManager* ui, AppState* state, int collection_index, int request_index);
bool ui_collections_handle_move_request(UIManager* ui, AppState* state, int source_collection, int request_index, int target_collection);

#ifdef __cplusplus
}
#endif

#endif