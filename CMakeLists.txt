#
#  CMakeLists.txt
#  asciiplay
#
#  Created by Piotr Panasewicz on 10/08/2026.
#  Copyright © 2026 Codice. All rights reserved.
#
#  Licensed under the MIT licence. See LICENSE in the project root
#  for the full text.
#
cmake_minimum_required(VERSION 3.16)
project(asciiplay VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to an optimised build -- the glyph matcher is the hot loop and an
# unoptimised build is roughly 10x slower, enough to stop video keeping up.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 Debug Release RelWithDebInfo MinSizeRel)
endif()

# Everything but main() is a library, so the unit tests link the same code
# the player runs.
add_library(asciiplay_core STATIC
    src/asciiart.cpp
    src/controls.cpp
    src/input.cpp
    src/workers.cpp
    src/platform_posix.cpp
    src/platform_win.cpp        # each guards itself with #ifdef; only one compiles
)
target_include_directories(asciiplay_core PUBLIC src)

add_executable(asciiplay src/main.cpp)
target_link_libraries(asciiplay PRIVATE asciiplay_core)
target_compile_definitions(asciiplay PRIVATE
                           ASCIIPLAY_VERSION="${PROJECT_VERSION}")

# ---- threads ---------------------------------------------------------------
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(asciiplay_core PUBLIC Threads::Threads)

# ---- per-compiler tuning ---------------------------------------------------
include(CheckCXXCompilerFlag)

# Applied to every target, so the tests are built the way the player is.
option(ASCIIPLAY_NATIVE "Tune for the building machine's CPU" OFF)

function(asciiplay_tune target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:/O2 /fp:fast>)
        target_compile_definitions(${target} PRIVATE
            _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wno-unused-parameter)
        target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:-O3 -ffast-math>)

        # Native tuning: the flag differs between x86 and Apple Silicon.  Off by
        # default, because a binary built with it runs only on CPUs at least as new
        # as the one that built it -- which is fine for your own machine and wrong
        # for anything anyone else installs (Homebrew bottles, distro packages, a
        # binary copied to another box).  Turn it on for a local build.
        if(ASCIIPLAY_NATIVE)
            check_cxx_compiler_flag("-march=native" HAVE_MARCH_NATIVE)
            if(HAVE_MARCH_NATIVE)
                target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:-march=native>)
            else()
                check_cxx_compiler_flag("-mcpu=native" HAVE_MCPU_NATIVE)
                if(HAVE_MCPU_NATIVE)   # Apple Silicon / arm64 clang
                    target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:-mcpu=native>)
                endif()
            endif()
        endif()
    endif()
endfunction()

asciiplay_tune(asciiplay_core)
asciiplay_tune(asciiplay)

# ---- unit tests ------------------------------------------------------------
# A plain executable with its own tiny runner: asciiplay links nothing, and
# its tests do not either.  ctest --test-dir build -C Release
option(ASCIIPLAY_TESTS "Build the unit tests" ON)
if(ASCIIPLAY_TESTS)
    enable_testing()
    add_executable(asciiplay_tests
        tests/test_main.cpp
        tests/test_input.cpp
        tests/test_controls.cpp
        tests/test_render.cpp
        tests/test_decoder.cpp
        tests/test_grid.cpp
        tests/test_workers.cpp
    )
    target_link_libraries(asciiplay_tests PRIVATE asciiplay_core)
    asciiplay_tune(asciiplay_tests)
    add_test(NAME unit COMMAND asciiplay_tests)
endif()

# ---- warn early if ffmpeg is missing ---------------------------------------
find_program(FFMPEG_EXECUTABLE ffmpeg)
find_program(FFPROBE_EXECUTABLE ffprobe)
if(NOT FFMPEG_EXECUTABLE OR NOT FFPROBE_EXECUTABLE)
    message(WARNING
        "ffmpeg/ffprobe not found on PATH. asciiplay builds fine, but needs "
        "both at runtime.  macOS: brew install ffmpeg   "
        "Windows: winget install Gyan.FFmpeg   Debian: apt install ffmpeg")
endif()

# ---- regenerate the baked-in glyph shapes ----------------------------------
# Optional: only needed to match a different terminal font.
#   cmake --build . --target glyphs
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    set(ASCIIPLAY_FONT "" CACHE FILEPATH "TTF to rasterise for 'glyphs' target")
    add_custom_target(glyphs
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_glyphs.py ${ASCIIPLAY_FONT}
                > ${CMAKE_CURRENT_SOURCE_DIR}/src/glyph_table.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Rasterising glyph coverage table (needs Pillow)"
        VERBATIM)
endif()

install(TARGETS asciiplay RUNTIME DESTINATION bin)
