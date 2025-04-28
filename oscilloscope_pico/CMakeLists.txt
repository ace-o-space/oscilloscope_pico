# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(oscilloscope_pico C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(oscilloscope_pico oscilloscope_pico.c)

pico_set_program_name(oscilloscope_pico "oscilloscope_pico")
pico_set_program_version(oscilloscope_pico "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(oscilloscope_pico 0)
pico_enable_stdio_usb(oscilloscope_pico 1)

# Add the standard library to the build
target_link_libraries(oscilloscope_pico
    pico_stdlib
    pico_ili9341
    hardware_adc
    hardware_dma
    hardware_pio
    hardware_spi
    pico_multicore
    display_driver
    adc_driver
    global_buffer
)
# Add the standard include files to the build
target_include_directories(oscilloscope_pico PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}
)


pico_add_extra_outputs(oscilloscope_pico)
add_subdirectory("${PROJECT_SOURCE_DIR}/pico_ili9341" "${PROJECT_BINARY_DIR}/pico_ili9341")
add_subdirectory("${PROJECT_SOURCE_DIR}/display_driver" "${PROJECT_BINARY_DIR}/display_driver")
add_subdirectory("${PROJECT_SOURCE_DIR}/adc_driver" "${PROJECT_BINARY_DIR}/adc_driver")
add_subdirectory("${PROJECT_SOURCE_DIR}/global_buffer" "${PROJECT_BINARY_DIR}/global_buffer")


