cmake_minimum_required(VERSION 3.13)

project(adc_driver)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PUBLIC
    "${PROJECT_SOURCE_DIR}/include/adc_driver/adc_driver.h"
    "${PROJECT_SOURCE_DIR}/src/adc_driver.c"
)

# Add any user requested libraries
target_link_libraries(${PROJECT_NAME} 
        pico_stdlib
        hardware_adc
        hardware_dma       
        )

target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/../global_buffer/include")