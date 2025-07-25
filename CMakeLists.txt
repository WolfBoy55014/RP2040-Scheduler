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

project(RP2040-Scheduler C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# set(CMAKE_CXX_FLAGS_DEBUG "-Og")
# set(CMAKE_ASM_FLAGS_DEBUG "-Og")
# set(CMAKE_C_FLAGS_DEBUG "-Og")

# Add executable. Default name is the project name, version 0.2
add_executable(RP2040-Scheduler main.c scheduler.c context.s)

pico_set_program_name(RP2040-Scheduler "RP2040-Scheduler")
pico_set_program_version(RP2040-Scheduler "0.2")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(RP2040-Scheduler 1)
pico_enable_stdio_usb(RP2040-Scheduler 0)

# Add the standard library to the build
target_link_libraries(RP2040-Scheduler
        pico_stdlib
        hardware_pwm)

# Add the standard include files to the build
target_include_directories(RP2040-Scheduler PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include/
)

pico_add_extra_outputs(RP2040-Scheduler)

