cmake_minimum_required(VERSION 3.13)

project(pico_ili9341)

add_library(${PROJECT_NAME} STATIC
    src/pico_ili9341.c
    src/graphics.c
    src/fonts.c
    )

target_sources(${PROJECT_NAME} PUBLIC 
    "${PROJECT_SOURCE_DIR}/include/pico_ili9341/pico_ili9341.h"
    "${PROJECT_SOURCE_DIR}/include/pico_ili9341/font_5x7.h"
    "${PROJECT_SOURCE_DIR}/include/pico_ili9341/font_8x8.h"
    "${PROJECT_SOURCE_DIR}/include/pico_ili9341/font_12x16.h"
    "${PROJECT_SOURCE_DIR}/src/pico_ili9341.c"
    "${PROJECT_SOURCE_DIR}/src/fonts.c"
    "${PROJECT_SOURCE_DIR}/src/graphics.c"
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_spi
    hardware_dma
    display_driver
)

#pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/ili9341.pio)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/../display_driver/include")