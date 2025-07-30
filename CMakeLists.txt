cmake_minimum_required(VERSION 3.16)
project(TinyRequest C)

set(CMAKE_C_STANDARD 11)

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
)

target_include_directories(TinyRequest PRIVATE
        include
        cjson
)

target_compile_definitions(TinyRequest PRIVATE
        PLATFORM_DESKTOP
        CURL_CURL_H
)

if(CMAKE_INSTALL_PREFIX)
    target_compile_definitions(TinyRequest PRIVATE "ASSET_PATH=\"${CMAKE_INSTALL_PREFIX}/share/tiny-request\"")
else()
    target_compile_definitions(TinyRequest PRIVATE "ASSET_PATH=\"assets\"")
endif()

target_link_libraries(TinyRequest
        -lraylib
        -lcurl
        -lcjson
        -lm
)

install(TARGETS TinyRequest DESTINATION bin)
install(DIRECTORY assets/ DESTINATION share/tiny-request)
