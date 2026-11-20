// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/document.h"

#include <QFont>

class QPainter;

namespace tratto::render {

// Font families behind core::TextFont (the bundled Inter for Sans).
[[nodiscard]] QStringList fontFamilies(core::TextFont font);

// Height (pt) of `box.text` wrapped to `box.bbox.width()`, with the box's style.
// At least one line, so an empty box still has a height.
[[nodiscard]] double textHeight(const core::TextBox &box);

// Paints the text at the box's top-left, wrapped to its width.
void paintTextBox(QPainter &painter, const core::TextBox &box);

} // namespace tratto::render
