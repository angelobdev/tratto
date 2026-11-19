// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/paper.h"

#include <QColor>
#include <QSizeF>

class QPainter;

namespace tratto::render {

// Line color of the paper pattern, from docs/DESIGN.md.
[[nodiscard]] QColor patternColor(const core::PaperStyle &style);

// Paints the paper color and its pattern for a page `pageSize` pt large.
// The painter's transform must map page pt to device pixels. Lines are
// 0.75 pt wide (1 px at 100% zoom) and scale with the page, but never get
// thinner than half a device pixel.
void paintPaper(QPainter &painter, const core::PaperStyle &style, QSizeF pageSize);

} // namespace tratto::render
