# SPDX-License-Identifier: GPL-3.0-or-later
qt_add_executable(tratto main.cpp)

set_source_files_properties(
    qml/Theme.qml
    qml/Format.qml
    qml/components/Icons.qml
    qml/library/FakeLibrary.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(tratto
    URI Tratto
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/Format.qml
        qml/components/AppButton.qml
        qml/components/AppSwitch.qml
        qml/components/BackButton.qml
        qml/components/CheckOption.qml
        qml/components/CompactComboBox.qml
        qml/components/Divider.qml
        qml/components/FocusRing.qml
        qml/components/Icon.qml
        qml/components/IconButton.qml
        qml/components/Icons.qml
        qml/components/NavItem.qml
        qml/components/ResponsiveGrid.qml
        qml/components/SearchInput.qml
        qml/components/SegmentedChoice.qml
        qml/editor/EditorPage.qml
        qml/editor/EditorTopBar.qml
        qml/editor/PagesPanel.qml
        qml/editor/ToolRow.qml
        qml/editor/ZoomControl.qml
        qml/library/FakeLibrary.qml
        qml/library/GroupCard.qml
        qml/library/LibraryPage.qml
        qml/library/LibrarySidebar.qml
        qml/library/NoteCard.qml
        qml/library/SortButton.qml
        qml/library/ViewToggle.qml
        qml/settings/AccentPreview.qml
        qml/settings/AccentSwatch.qml
        qml/settings/AppearanceSection.qml
        qml/settings/GeneralSection.qml
        qml/settings/OptionGroup.qml
        qml/settings/OptionRow.qml
        qml/settings/SettingsPage.qml
        qml/settings/ThemeCard.qml
    SOURCES
        appearance.h appearance.cpp
        localization.h localization.cpp
        preferences.h preferences.cpp
        appsettings.h
        paperthumbnail.h paperthumbnail.cpp
        editortools.h editortools.cpp
        notesession.h notesession.cpp
        canvasitem.h canvasitem.cpp
)

# Inter (SIL Open Font License, see fonts/OFL.txt), loaded at startup.
qt_add_resources(tratto "fonts"
    PREFIX /fonts
    BASE fonts
    FILES fonts/InterVariable.ttf)

target_compile_definitions(tratto PRIVATE
    TRATTO_VERSION="${PROJECT_VERSION}"
    TRATTO_APP_ID="${TRATTO_APP_ID}")

target_link_libraries(tratto PRIVATE
    tratto_core
    tratto_render
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2)
tratto_set_warnings(tratto)

# English is the source language; i18n/tratto_it.ts is compiled and embedded under :/i18n.
# Refresh the .ts files after changing strings: cmake --build build --target update_translations
qt_add_translations(tratto
    TS_FILE_BASE tratto
    TS_FILE_DIR ${PROJECT_SOURCE_DIR}/i18n
    RESOURCE_PREFIX /i18n
    LUPDATE_OPTIONS -no-obsolete)

set_target_properties(tratto PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE)
