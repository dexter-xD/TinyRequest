cmake_minimum_required(VERSION 3.16)
project(TinyRequest C)

set(CMAKE_C_STANDARD 11)

set(LIBS_DIR "C:/libs")
set(CJSON_DIR "${LIBS_DIR}/cJSON-1.7.18")
set(CURL_DIR "${LIBS_DIR}/curl-8.15")
set(RAYLIB_DIR "${LIBS_DIR}/raylib-5.5")

add_executable(TinyRequest
        main.c
        src/http_client.c
        src/ui_components.c
        src/theme.c
        src/request_state.c
        src/error_handler.c
        src/memory_manager.c
        src/json_processor.c
        src/ui/ui_style.c
        ${CJSON_DIR}/cJSON.c
)

target_include_directories(TinyRequest PRIVATE
        include
        ${CJSON_DIR}
        ${CURL_DIR}/include
        ${RAYLIB_DIR}/include
)

target_link_directories(TinyRequest PRIVATE
        ${CURL_DIR}/lib
        ${RAYLIB_DIR}/lib
)

target_compile_definitions(TinyRequest PRIVATE
        WIN32_LEAN_AND_MEAN
        PLATFORM_DESKTOP
        NOGDI
        NOUSER
        CURL_CURL_H
        CURL_STATICLIB
)

target_link_libraries(TinyRequest
        raylib
        curl
        ssl
        crypto
        z
        ws2_32
        wldap32
        crypt32
        normaliz
        winmm
        gdi32
        kernel32
        user32
        winspool
        shell32
        ole32
        oleaut32
        uuid
        comdlg32
        advapi32
)

add_custom_command(TARGET TinyRequest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CURL_DIR}/bin/libcurl-x64.dll"
    $<TARGET_FILE_DIR:TinyRequest>
    COMMENT "Copying libcurl DLL to output directory"
)

