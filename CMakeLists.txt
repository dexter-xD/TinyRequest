cmake_minimum_required(VERSION 3.16)
project(TinyRequest C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find required packages using pkg-config
find_package(PkgConfig REQUIRED)

# Find OpenGL (required for ImGui)
find_package(OpenGL REQUIRED)

# Find GLFW3
pkg_check_modules(GLFW3 REQUIRED glfw3)

# Find libcurl
pkg_check_modules(CURL REQUIRED libcurl)

# Try to find system cJSON, fallback to bundled version
pkg_check_modules(CJSON libcjson)

# External library paths
set(CIMGUI_DIR ${CMAKE_SOURCE_DIR}/externals/cimgui)
set(CJSON_DIR ${CMAKE_SOURCE_DIR}/externals/cJSON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CIMGUI_DIR}
    ${CIMGUI_DIR}/imgui
    ${CIMGUI_DIR}/imgui/backends
)

# Add system library include directories
include_directories(${GLFW3_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})

# If system cJSON is available, use it; otherwise use bundled version
if(CJSON_FOUND)
    include_directories(${CJSON_INCLUDE_DIRS})
    message(STATUS "Using system cJSON library")
else()
    include_directories(${CJSON_DIR})
    message(STATUS "Using bundled cJSON library")
endif()

# cJSON source files (only if system cJSON is not available)
if(NOT CJSON_FOUND)
    set(CJSON_SOURCES
        ${CJSON_DIR}/cJSON.c
        ${CJSON_DIR}/cJSON_Utils.c
    )
endif()

# Build ImGui directly (C++ approach)
set(IMGUI_SOURCES
    ${CIMGUI_DIR}/imgui/imgui.cpp
    ${CIMGUI_DIR}/imgui/imgui_demo.cpp
    ${CIMGUI_DIR}/imgui/imgui_draw.cpp
    ${CIMGUI_DIR}/imgui/imgui_tables.cpp
    ${CIMGUI_DIR}/imgui/imgui_widgets.cpp
    ${CIMGUI_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${CIMGUI_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)

# Create imgui static library
add_library(imgui STATIC ${IMGUI_SOURCES})

# Configure imgui
target_include_directories(imgui PUBLIC
    ${CIMGUI_DIR}/imgui
    ${CIMGUI_DIR}/imgui/backends
    ${GLFW3_INCLUDE_DIRS}
)

# Link GLFW to imgui library since imgui backends need GLFW functions
target_link_libraries(imgui
    ${GLFW3_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

# Add compiler flags for GLFW
target_compile_options(imgui PRIVATE ${GLFW3_CFLAGS_OTHER})

# Application source files
set(APP_SOURCES
    main.cpp
    src/app_state.c
    src/collections.c
    src/http_client.c
    src/ui_manager.cpp
    src/request_response.c
    src/persistence.c
    src/font_awesome.cpp
    src/app/app_core.cpp
    src/app/app_theme.cpp
    src/app/app_window.cpp
    src/ui/ui_core.cpp
    src/ui/ui_dialogs.cpp
    src/ui/ui_main_tabs.cpp
    src/ui/ui_panels.cpp
    src/ui/ui_request_panel.cpp
    src/ui/ui_response_panel.cpp
    src/ui/theme.cpp
)

include_directories(include)

# Create main executable
add_executable(TinyRequest
    ${APP_SOURCES}
    ${CJSON_SOURCES}
)

# Create a symlink or copy with lowercase name for packaging consistency
add_custom_command(TARGET TinyRequest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:TinyRequest> $<TARGET_FILE_DIR:TinyRequest>/tinyrequest
    COMMENT "Creating lowercase binary name for packaging"
)

# Link libraries
target_link_libraries(TinyRequest
    imgui
    ${CURL_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

# If using system cJSON, link it
if(CJSON_FOUND)
    target_link_libraries(TinyRequest ${CJSON_LIBRARIES})
endif()

# Add compiler flags
target_compile_options(TinyRequest PRIVATE 
    ${GLFW3_CFLAGS_OTHER}
    ${CURL_CFLAGS_OTHER}
)

# Add linker flags
target_link_options(TinyRequest PRIVATE 
    ${GLFW3_LDFLAGS_OTHER}
    ${CURL_LDFLAGS_OTHER}
)

# Linux-specific linking
if(UNIX AND NOT APPLE)
    target_link_libraries(TinyRequest
        GL
        X11
        Xrandr
        Xinerama
        Xcursor
        Xi
        pthread
        dl
    )
endif()

# Debug/Release configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(TinyRequest PRIVATE -g -O0)
else()
    target_compile_options(TinyRequest PRIVATE -O2 -DNDEBUG)
endif()

# Compiler-specific flags for Linux
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    target_compile_definitions(TinyRequest PRIVATE
        CIMGUI_DEFINE_ENUMS_AND_STRUCTS
        CIMGUI_USE_GLFW
        CIMGUI_USE_OPENGL3
    )
    target_compile_options(TinyRequest PRIVATE -Wall -Wextra)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_definitions(TinyRequest PRIVATE
        CIMGUI_DEFINE_ENUMS_AND_STRUCTS
        CIMGUI_USE_GLFW
        CIMGUI_USE_OPENGL3
    )
    target_compile_options(TinyRequest PRIVATE -Wall -Wextra)
endif()

# Copy assets directory to build directory
add_custom_command(TARGET TinyRequest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:TinyRequest>/assets
    COMMENT "Copying assets directory to build output"
)

# Print configuration information
message(STATUS "Build configuration:")
message(STATUS "  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "  GLFW3 version: ${GLFW3_VERSION}")
message(STATUS "  CURL version: ${CURL_VERSION}")
if(CJSON_FOUND)
    message(STATUS "  cJSON version: ${CJSON_VERSION}")
else()
    message(STATUS "  cJSON: Using bundled version")
endif()