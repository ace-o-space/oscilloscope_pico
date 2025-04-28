cmake_minimum_required(VERSION 3.13)

project(global_buffer)

add_library(${PROJECT_NAME} STATIC
    src/global_buffer.c)

target_sources(${PROJECT_NAME} PUBLIC
    "${PROJECT_SOURCE_DIR}/include/global_buffer/global_buffer.h"
    "${PROJECT_SOURCE_DIR}/src/global_buffer.c"
)

# Add any user requested libraries
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_sync
    )

target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
