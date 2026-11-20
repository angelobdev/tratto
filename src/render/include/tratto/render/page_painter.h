// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/document.h"

#include <QRectF>

class QPainter;
class QPainterPath;

namespace tratto::render {

// The filled outline of a stroke, in page pt.
[[nodiscard]] QPainterPath strokePath(const core::Stroke &stroke);

// Paints one stroke. Highlighter strokes use kHighlighterAlpha.
void paintStroke(QPainter &painter, const core::Stroke &stroke);

// Paints a page: paper, then highlighter strokes, then ink, then text (ARCHITECTURE §6).
// The painter's transform maps page pt to device pixels. Only objects whose
// bounds intersect `clip` (page pt) are drawn; an empty clip draws everything.
void paintPage(QPainter &painter, const core::Page &page, const QRectF &clip = { });

} // namespace tratto::render
