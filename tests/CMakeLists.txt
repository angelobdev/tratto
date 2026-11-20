# SPDX-License-Identifier: GPL-3.0-or-later
# tratto_add_test(<name> SOURCES ... LIBS ...): one Qt Test executable, run headless.
function(tratto_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})
    qt_add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARG_LIBS})
    tratto_set_warnings(${name})
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

tratto_add_test(tst_color SOURCES core/tst_color.cpp LIBS tratto_core)
tratto_add_test(tst_paper SOURCES core/tst_paper.cpp LIBS tratto_core)
tratto_add_test(tst_command SOURCES core/tst_command.cpp LIBS tratto_core)
tratto_add_test(tst_stroke_geometry SOURCES core/tst_stroke_geometry.cpp LIBS tratto_core)
tratto_add_test(tst_paper_painter SOURCES render/tst_paper_painter.cpp LIBS tratto_render)
tratto_add_test(tst_page_painter SOURCES render/tst_page_painter.cpp LIBS tratto_render)
tratto_add_test(tst_storage SOURCES storage/tst_storage.cpp LIBS tratto_storage)

# The canvas and its collaborators, compiled straight into the test.
set(TRATTO_APP_DIR ${PROJECT_SOURCE_DIR}/src/app)
tratto_add_test(tst_canvas
    SOURCES app/tst_canvas.cpp
        ${TRATTO_APP_DIR}/canvasitem.h ${TRATTO_APP_DIR}/canvasitem.cpp
        ${TRATTO_APP_DIR}/editortools.h ${TRATTO_APP_DIR}/editortools.cpp
        ${TRATTO_APP_DIR}/notesession.h ${TRATTO_APP_DIR}/notesession.cpp
        ${TRATTO_APP_DIR}/preferences.h ${TRATTO_APP_DIR}/preferences.cpp
        ${TRATTO_APP_DIR}/library.h ${TRATTO_APP_DIR}/library.cpp
    LIBS tratto_render tratto_storage Qt6::Quick)
target_include_directories(tst_canvas PRIVATE ${TRATTO_APP_DIR})
# Keep the test from touching the user's settings.
set_property(TEST tst_canvas APPEND PROPERTY ENVIRONMENT "XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-config")

tratto_add_test(tst_library
    SOURCES app/tst_library.cpp
        ${TRATTO_APP_DIR}/library.h ${TRATTO_APP_DIR}/library.cpp
        ${TRATTO_APP_DIR}/notesession.h ${TRATTO_APP_DIR}/notesession.cpp
    LIBS tratto_render tratto_storage Qt6::Qml)
target_include_directories(tst_library PRIVATE ${TRATTO_APP_DIR})
