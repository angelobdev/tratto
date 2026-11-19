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
tratto_add_test(tst_paper_painter SOURCES render/tst_paper_painter.cpp LIBS tratto_render)
