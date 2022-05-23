cmake_minimum_required(VERSION 3.12)

# Set build type to reduce firmware size
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel)
endif()

# Set main target and component locations
set(DYNRUNTIME_TARGET dynruntime)
get_filename_component(MICROPY_DIR ${MPY_DIR} ABSOLUTE)

if (PICO_SDK_PATH_OVERRIDE)
    set(PICO_SDK_PATH ${PICO_SDK_PATH_OVERRIDE})
else()
    set(PICO_SDK_PATH ../../lib/pico-sdk)
endif()

# Set the PICO_BOARD if it's not already set (allow a board to override it).
if(NOT PICO_BOARD)
    string(TOLOWER ${MICROPY_BOARD} PICO_BOARD)
endif()

# Include component cmake fragments
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

set(PICO_TINYUSB_PATH ${MICROPY_DIR}/lib/tinyusb)

set(MICROPY_PORT_DIR ${MICROPY_DIR}/ports/rp2)
set(MICROPY_BOARD_DIR ${MICROPY_DIR}/ports/rp2/boards/PICO)

set(CONFIG_H ${CMAKE_CURRENT_BINARY_DIR}/config.h)


set(PYTHON python3)
set(MPY_CROSS ${MICROPY_DIR}/mpy-cross/mpy-cross)
set(MPY_TOOL ${PYTHON} ${MICROPY_DIR}/tools/mpy-tool.py)
set(MPY_LD ${PYTHON} ${MICROPY_DIR}/tools/mpy_ld.py)



# Define the top-level project
project(${DYNRUNTIME_TARGET})

pico_sdk_init()

set(MICROPY_CROSS_FLAGS -march=armv7m)


    # hardware_sync # -- .data.striped_spin_lock_num non-empty
    # pico_sync
    #pico_base_headers
    #pico_binary_info
    #pico_bootrom
    #pico_multicore
    #pico_platform
    #pico_stdio
    #pico_stdlib
    #pico_time
    #pico_unique_id
    #pico_util
    #tinyusb_common
    #tinyusb_device


    #hardware_adc
    #hardware_base
    #hardware_clocks
    #hardware_dma
    #hardware_flash
    #hardware_gpio
    #hardware_i2c
    #hardware_irq
    #hardware_pio
    #hardware_pwm
    #hardware_regs
    #hardware_rtc
    #hardware_spi
    #hardware_structs
    #hardware_timer
    #hardware_uart
    #hardware_watchdog

set(PICO_SDK_COMPONENTS
    hardware_irq
    hardware_gpio
)

set_source_files_properties(
    ${PICO_SDK_PATH}/src/rp2_common/pico_double/double_math.c
    ${PICO_SDK_PATH}/src/rp2_common/pico_float/float_math.c
    PROPERTIES
    COMPILE_OPTIONS "-Wno-error=uninitialized"
)

add_library(${DYNRUNTIME_TARGET} OBJECT)

set(DYNRUNTIME_TARGET_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/init.c
    ${CMAKE_CURRENT_LIST_DIR}/module.cpp
)

target_sources(${DYNRUNTIME_TARGET} PRIVATE
    ${CONFIG_H}
    ${DYNRUNTIME_TARGET_SOURCES}
)


# TODO make this a custom command
# python3 ../micropython-v1.18/tools/mpy_ld.py --arch armv7m --preprocess -o config.h init.c
add_custom_command(
    PRE_BUILD
    DEPENDS ${DYNRUNTIME_TARGET_SOURCES}
    OUTPUT ${CONFIG_H}
    COMMAND ${MPY_LD} --arch armv7m --preprocess -o ${CONFIG_H} ${DYNRUNTIME_TARGET_SOURCES}
)

target_compile_definitions(${DYNRUNTIME_TARGET} PRIVATE
    NDEBUG
    NO_QSTR
    MICROPY_ENABLE_DYNRUNTIME
    MP_CONFIGFILE="${CONFIG_H}"

    FFCONF_H=\"${MICROPY_OOFATFS_DIR}/ffconf.h\"
    LFS1_NO_MALLOC LFS1_NO_DEBUG LFS1_NO_WARN LFS1_NO_ERROR LFS1_NO_ASSERT
    LFS2_NO_MALLOC LFS2_NO_DEBUG LFS2_NO_WARN LFS2_NO_ERROR LFS2_NO_ASSERT
    PICO_FLOAT_PROPAGATE_NANS=1
    PICO_STACK_SIZE=0x2000
    PICO_CORE1_STACK_SIZE=0
    PICO_PROGRAM_NAME="${DYNRUNTIME_TARGET}"
    PICO_NO_PROGRAM_VERSION_STRING=1 # do it ourselves in main.c
    MICROPY_BUILD_TYPE="${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} ${CMAKE_BUILD_TYPE}"
    PICO_NO_BI_STDIO_UART=1 # we call it UART REPL
    PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1
)

target_include_directories(${DYNRUNTIME_TARGET} PRIVATE
    .
    ${MICROPY_DIR}
    ${MICROPY_PORT_DIR}
    ${MICROPY_BOARD_DIR}
    ${PICO_SDK_PATH}
)

target_compile_options(${DYNRUNTIME_TARGET} PRIVATE
    -I. -I${MICROPY_DIR}
    -Os
    -Wall -Werror
    -fpic -fno-common
    -U _FORTIFY_SOURCE
)

# dynruntime will always produce fpermissive warnings
set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/module.cpp
    PROPERTIES
    COMPILE_OPTIONS "-fpermissive;-Wno-error;-std=c++17"
)

target_link_libraries(${DYNRUNTIME_TARGET} PRIVATE
    ${PICO_SDK_COMPONENTS}
)

add_library(${DYNRUNTIME_TARGET}.mpy STATIC $<TARGET_OBJECTS:${DYNRUNTIME_TARGET}>)

add_custom_command(
    TARGET ${DYNRUNTIME_TARGET}.mpy
    POST_BUILD
    DEPENDS ${DYNRUNTIME_TARGET}.o
    COMMAND ${MPY_LD} --arch armv7m --qstrs ${CONFIG_H} -o ${DYNRUNTIME_TARGET}.mpy $<TARGET_OBJECTS:${DYNRUNTIME_TARGET}>
    COMMAND_EXPAND_LISTS
)