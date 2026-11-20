# SPDX-License-Identifier: GPL-3.0-or-later
# tratto_storage: note bundles on disk, format and migrations. Links QtGui (thumbnails) —
# never Quick or Qml.
add_library(tratto_storage STATIC
    include/tratto/storage/format.h
    format.cpp
    include/tratto/storage/note_store.h
    note_store.cpp
)
target_include_directories(tratto_storage PUBLIC include)
target_link_libraries(tratto_storage PUBLIC tratto_core Qt6::Gui)
tratto_set_warnings(tratto_storage)
