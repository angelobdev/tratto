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
tratto_add_test(tst_selection SOURCES core/tst_selection.cpp LIBS tratto_core)
tratto_add_test(tst_paper_painter SOURCES render/tst_paper_painter.cpp LIBS tratto_render)
tratto_add_test(tst_page_painter SOURCES render/tst_page_painter.cpp LIBS tratto_render)
tratto_add_test(tst_text_painter SOURCES render/tst_text_painter.cpp LIBS tratto_render)
tratto_add_test(tst_storage SOURCES storage/tst_storage.cpp LIBS tratto_storage)

# App-level tests link the app library; tst_ui also loads its QML module.
tratto_add_test(tst_canvas SOURCES app/tst_canvas.cpp LIBS tratto_ui)
# Keep the test from touching the user's settings.
set_property(TEST tst_canvas APPEND PROPERTY ENVIRONMENT "XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-config")
tratto_add_test(tst_library SOURCES app/tst_library.cpp LIBS tratto_ui)
tratto_add_test(tst_ui SOURCES app/tst_ui.cpp LIBS tratto_ui tratto_uiplugin)
set_property(TEST tst_ui APPEND PROPERTY ENVIRONMENT "XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-config")
