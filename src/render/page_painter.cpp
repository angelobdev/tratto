// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/render/page_painter.h"

#include "tratto/core/stroke_geometry.h"
#include "tratto/core/tools.h"
#include "tratto/render/paper_painter.h"

#include <QPainter>
#include <QPainterPath>

namespace tratto::render {

namespace {

QColor strokeColor(const core::Stroke &stroke)
{
    QColor color(stroke.color.r, stroke.color.g, stroke.color.b);
    if (stroke.tool == core::StrokeTool::Highlighter)
        color.setAlphaF(static_cast<float>(core::kHighlighterAlpha));
    return color;
}

} // namespace

QPainterPath strokePath(const core::Stroke &stroke)
{
    const bool usePressure = stroke.tool == core::StrokeTool::Pen;
    const std::vector<QPointF> outline = core::outline(stroke.points, stroke.width, usePressure);
    QPainterPath path;
    path.setFillRule(Qt::WindingFill); // overlapping loops at sharp turns stay filled
    if (outline.empty())
        return path;
    path.moveTo(outline.front());
    for (std::size_t i = 1; i < outline.size(); ++i)
        path.lineTo(outline[i]);
    path.closeSubpath();
    return path;
}

void paintStroke(QPainter &painter, const core::Stroke &stroke)
{
    painter.fillPath(strokePath(stroke), strokeColor(stroke));
}

void paintPage(QPainter &painter, const core::Page &page, const QRectF &clip)
{
    paintPaper(painter, page.paper, page.size);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    const auto visible = [&](const core::Object &o) {
        return clip.isEmpty() || core::objectBounds(o).intersects(clip);
    };
    // Highlighter below ink, each in drawing order.
    for (const core::StrokeTool layer : { core::StrokeTool::Highlighter, core::StrokeTool::Pen }) {
        for (const core::Object &object : page.objects) {
            const auto *stroke = std::get_if<core::Stroke>(&object);
            if (stroke && stroke->tool == layer && visible(object))
                paintStroke(painter, *stroke);
        }
    }
    painter.restore();
}

} // namespace tratto::render
