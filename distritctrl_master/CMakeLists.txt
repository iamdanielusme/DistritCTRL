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
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(distritctrl_master C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()


if (TARGET tinyusb_device)
    add_executable(distritctrl_master
        main.c
        usb_descriptors.c
        midi_core.c
        display_oled.c
        sh1106.c
        led_ring.c
        step_sequencer.c
        slave_link.c
        ctrl_protocol.c
        fader_driver.c
        ultra_driver.c
    )
    
    # Para que encuentre tusb_config.h y headers locales
    target_include_directories(distritctrl_master PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
    )
    #pico_generate_pio_header(distritctrl_master ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)
    target_link_libraries(distritctrl_master
        pico_stdlib
        tinyusb_device
        tinyusb_board
        hardware_adc
        hardware_i2c
        hardware_pio
        hardware_clocks

    )

    # No necesitamos stdio por USB/UART en este proyecto
    pico_enable_stdio_usb(distritctrl_master 1)
    pico_enable_stdio_uart(distritctrl_master 0)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(distritctrl_master)

elseif(PICO_ON_DEVICE)
    message("Skipping distritctrl_master because TinyUSB submodule is not initialized in the SDK")
endif()