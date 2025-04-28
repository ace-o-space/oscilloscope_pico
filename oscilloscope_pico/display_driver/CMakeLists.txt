cmake_minimum_required(VERSION 3.13)

project(display_driver)

add_library(${PROJECT_NAME} STATIC 
    src/display_driver.c)

target_sources(${PROJECT_NAME} PUBLIC
    "${PROJECT_SOURCE_DIR}/include/display_driver/display_driver.h"
    "${PROJECT_SOURCE_DIR}/src/display_driver.c"
)

# Add any user requested libraries
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    hardware_spi
    hardware_adc
    hardware_dma
    pico_ili9341
    global_buffer
    )

target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/../pico_ili9341/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/../global_buffer/include")
