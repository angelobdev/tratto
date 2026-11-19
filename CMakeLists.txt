# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.21)

project(Tratto
    VERSION 0.1.0
    DESCRIPTION "Handwritten notes and PDF annotation"
    HOMEPAGE_URL "https://github.com/angelobdev/tratto"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(TRATTO_APP_ID "io.github.angelobdev.Tratto")

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 LinguistTools Test)
qt_standard_project_setup(
    REQUIRES 6.8
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES it)

include(cmake/CompilerWarnings.cmake)

enable_testing()

add_subdirectory(src/core)
add_subdirectory(src/app)
add_subdirectory(tests)

# Optional: `cmake --build build --target format`
find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE TRATTO_FORMAT_SOURCES CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.h
        ${PROJECT_SOURCE_DIR}/tests/*.cpp ${PROJECT_SOURCE_DIR}/tests/*.h)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${TRATTO_FORMAT_SOURCES}
        COMMENT "Formatting sources with clang-format")
endif()
