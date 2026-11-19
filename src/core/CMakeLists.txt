# SPDX-License-Identifier: GPL-3.0-or-later
# tratto_core: data model and pure logic. Links QtCore only — never GUI or Quick.
add_library(tratto_core STATIC
    include/tratto/core/color.h
    color.cpp
    include/tratto/core/paper.h
    paper.cpp
    include/tratto/core/tools.h
)
target_include_directories(tratto_core PUBLIC include)
target_link_libraries(tratto_core PUBLIC Qt6::Core)
tratto_set_warnings(tratto_core)
