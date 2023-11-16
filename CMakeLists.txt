cmake_minimum_required(VERSION 3.22)

project(
    "Super Shuckie"
    LANGUAGES CXX C
)

if(NOT DEFINED SAMEBOY_DIR)
    set(SAMEBOY_DIR "SAMEBOY_DIR_NOT_SET" CACHE INTERNAL PATH)
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(SDL2 REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(supershuckie
    src/main.cpp
    src/file_rw.cpp
    src/emulator_window.cpp
    src/emulator_window_menu.cpp
    src/gameboy_context.cpp
    src/settings.cpp
    src/input_device.cpp
    src/pixel_buffer_view.cpp
    src/speed_settings_window.cpp
    src/sudo_gameboy.c
    src/controls_settings_window.cpp
)

add_library(sameboy STATIC
    "${SAMEBOY_DIR}/Core/apu.c"
    "${SAMEBOY_DIR}/Core/camera.c"
    "${SAMEBOY_DIR}/Core/cheats.c"
    "${SAMEBOY_DIR}/Core/debugger.c"
    "${SAMEBOY_DIR}/Core/display.c"
    "${SAMEBOY_DIR}/Core/gb.c"
    "${SAMEBOY_DIR}/Core/joypad.c"
    "${SAMEBOY_DIR}/Core/mbc.c"
    "${SAMEBOY_DIR}/Core/memory.c"
    "${SAMEBOY_DIR}/Core/printer.c"
    "${SAMEBOY_DIR}/Core/random.c"
    "${SAMEBOY_DIR}/Core/rewind.c"
    "${SAMEBOY_DIR}/Core/rumble.c"
    "${SAMEBOY_DIR}/Core/save_state.c"
    "${SAMEBOY_DIR}/Core/sgb.c"
    "${SAMEBOY_DIR}/Core/sm83_cpu.c"
    "${SAMEBOY_DIR}/Core/sm83_disassembler.c"
    "${SAMEBOY_DIR}/Core/symbol_hash.c"
    "${SAMEBOY_DIR}/Core/timing.c"
    "${SAMEBOY_DIR}/Core/workboy.c"
)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(CONSOLE_ARG "-mconsole")
else()
    set(CONSOLE_ARG "-mconsole")
endif()

target_link_libraries(supershuckie Qt6::Widgets sameboy ${SDL2_STATIC_LIBRARIES} "${CONSOLE_ARG}" -lreplay_recorder_c -ludp_command_handler)
target_include_directories(supershuckie
    PUBLIC ${Qt6Widgets_INCLUDE_DIRS}
    PUBLIC "${SAMEBOY_DIR}/Core"
)

target_compile_definitions(supershuckie PRIVATE _FORTIFY_SOURCES=0)
target_compile_definitions(sameboy PRIVATE GB_INTERNAL GB_VERSION=\"SuperShuckie64\")

if(WIN32)
    set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -lssp")
    set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} -lssp")
endif()

