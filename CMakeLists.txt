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
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(RP2040-Scheduler C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# set(CMAKE_CXX_FLAGS_DEBUG "-Og")
# set(CMAKE_ASM_FLAGS_DEBUG "-Og")
# set(CMAKE_C_FLAGS_DEBUG "-Og")

# ============================================================
# KERNEL LIBRARY TARGET
# This can be linked by external projects
# ============================================================

add_library(rp2040_kernel INTERFACE)

target_sources(rp2040_kernel INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/scheduler.c
    ${CMAKE_CURRENT_LIST_DIR}/src/context.s
    ${CMAKE_CURRENT_LIST_DIR}/src/channel.c
    ${CMAKE_CURRENT_LIST_DIR}/src/spinlock.c
    ${CMAKE_CURRENT_LIST_DIR}/src/com_channel_protocol.c
)

target_include_directories(rp2040_kernel INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include/
    ${CMAKE_CURRENT_LIST_DIR}/include/scheduler/
    ${CMAKE_CURRENT_LIST_DIR}/include/channel/
    ${CMAKE_CURRENT_LIST_DIR}/include/spinlock/
    ${CMAKE_CURRENT_LIST_DIR}/include/lib/
)

target_link_libraries(rp2040_kernel INTERFACE
    pico_stdlib
    hardware_pwm
    pico_multicore
)

# ============================================================
# TEST EXECUTABLE (Optional - only built if this is the main project)
# ============================================================

# Only build test executable if this is the top-level project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    
    add_executable(RP2040-Scheduler
        test/main.c
        test/md5.c
    )

    pico_set_program_name(RP2040-Scheduler "RP2040-Scheduler")
    pico_set_program_version(RP2040-Scheduler "0.1.2-pre-alpha")

    # Modify the below lines to enable/disable output over UART/USB
    pico_enable_stdio_uart(RP2040-Scheduler 1)
    pico_enable_stdio_usb(RP2040-Scheduler 0)

    # Link the kernel library to the test executable
    target_link_libraries(RP2040-Scheduler
        rp2040_kernel
        hardware_spi
    )

    # Add test-specific include directories
    target_include_directories(RP2040-Scheduler PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/test/include/
    )

    pico_add_extra_outputs(RP2040-Scheduler)

endif()
