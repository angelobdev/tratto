# SPDX-License-Identifier: GPL-3.0-or-later
qt_add_executable(tratto main.cpp)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(tratto
    URI Tratto
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/LibraryPage.qml
        qml/EditorPage.qml
    SOURCES
        appearance.h appearance.cpp
        localization.h localization.cpp
)

target_compile_definitions(tratto PRIVATE
    TRATTO_VERSION="${PROJECT_VERSION}"
    TRATTO_APP_ID="${TRATTO_APP_ID}")

target_link_libraries(tratto PRIVATE
    tratto_core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2)
tratto_set_warnings(tratto)

# English is the source language; i18n/tratto_it.ts is compiled and embedded under :/i18n.
# Refresh the .ts files after changing strings: cmake --build build --target update_translations
qt_add_translations(tratto
    TS_FILE_BASE tratto
    TS_FILE_DIR ${PROJECT_SOURCE_DIR}/i18n
    RESOURCE_PREFIX /i18n)

set_target_properties(tratto PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE)
