#include "ui/ui_core.h"
#include "ui/ui_manager.h"
#include "ui/ui_main_tabs.h"
#include "ui/ui_request_panel.h"
#include "ui/ui_response_panel.h"
#include "ui/ui_dialogs.h"
#include "ui/theme.h"
#include "font_awesome.h"
#include "app_state.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdlib.h>
#include <string.h>

extern "C" {

UIManager* ui_core_create(void) {
    UIManager* ui = (UIManager*)malloc(sizeof(UIManager));
    if (!ui) {
        return NULL;
    }

    memset(ui, 0, sizeof(UIManager));

    return ui;
}

void ui_core_destroy(UIManager* ui) {
    if (ui) {
        free(ui);
    }
}

int ui_core_init(UIManager* ui, GLFWwindow* window) {
    if (!ui || !window) {
        return -1;
    }

    ui->imgui_context = ImGui::CreateContext();
    if (!ui->imgui_context) {
        return -1;
    }

    ui->io = &ImGui::GetIO();

    ui->io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    const char* glsl_version = "#version 330 core";
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        ImGui::DestroyContext(ui->imgui_context);
        return -1;
    }

    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(ui->imgui_context);
        return -1;
    }

    ImGui::StyleColorsDark();

    return 0;
}

void ui_core_cleanup(UIManager* ui) {
    if (!ui) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    if (ui->imgui_context) {
        ImGui::DestroyContext(ui->imgui_context);
        ui->imgui_context = NULL;
    }
}

void ui_core_render(UIManager* ui, AppState* state) {
    if (!ui || !state) {
        return;
    }

    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(display_size);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

    ImGui::Begin("TinyRequest", NULL,
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoTitleBar);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));
    ImGui::BeginChild("MainContent", ImVec2(0, 0), false, ImGuiWindowFlags_None);

    ui_main_tabs_render(ui, state);

    ImGui::EndChild(); 
    ImGui::PopStyleVar(); 

    ImGui::End();
    ImGui::PopStyleVar(2); 

    if (state->show_save_dialog) {
        ui_dialogs_render_save_dialog(ui, state);
    }

    if (state->show_load_dialog) {
        ui_dialogs_render_load_dialog(ui, state);
    }

    if (state->show_cookie_manager) {
        ui_dialogs_render_cookie_manager(ui, state);
    }
}

void ui_core_update_from_state(UIManager* ui, const AppState* state) {
    if (!ui || !state) {
        return;
    }

    if (app_state_needs_request_sync((AppState*)state)) {
        app_state_sync_request_to_ui((AppState*)state);
    }
}

const char* ui_core_get_method_string(int method_index) {
    const char* methods[] = { "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS" };
    const int method_count = sizeof(methods) / sizeof(methods[0]);

    if (method_index >= 0 && method_index < method_count) {
        return methods[method_index];
    }
    return "GET"; 
}

int ui_core_get_method_index(const char* method) {
    const char* methods[] = { "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS" };
    const int method_count = sizeof(methods) / sizeof(methods[0]);

    if (!method) {
        return 0; 
    }

    for (int i = 0; i < method_count; i++) {
        if (strcmp(method, methods[i]) == 0) {
            return i;
        }
    }
    return 0; 
}

} 
