# SPDX-License-Identifier: GPL-3.0-or-later
# tratto_render: QPainter rendering of pages and objects. Links QtGui — never Quick or Qml.
add_library(tratto_render STATIC
    include/tratto/render/paper_painter.h
    paper_painter.cpp
    include/tratto/render/page_painter.h
    page_painter.cpp
)
target_include_directories(tratto_render PUBLIC include)
target_link_libraries(tratto_render PUBLIC tratto_core Qt6::Gui)
tratto_set_warnings(tratto_render)
