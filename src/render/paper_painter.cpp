// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/render/paper_painter.h"

#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

namespace tratto::render {

namespace {

using core::PatternKind;

// Line width and dot radius: 1 px at 100% zoom (1 px = 0.75 pt). They scale
// with the page, so zoomed-out pages and thumbnails get finer lines, down to
// half a device pixel so the pattern never disappears.
constexpr double kLineWidthPt = 0.75;
constexpr double kMinDevicePixels = 0.5;
constexpr double kDotRadiusPt = 0.75;

QColor toQColor(core::Rgba c)
{
    return QColor(c.r, c.g, c.b, c.a);
}

// Device pixels per pt along x for the current painter transform.
double deviceScale(const QPainter &painter)
{
    const QTransform t = painter.worldTransform();
    return std::max(std::hypot(t.m11(), t.m12()), 1e-6);
}

void paintRuled(QPainter &painter, QSizeF size, double spacing)
{
    QPainterPath lines;
    for (double y = core::ruledTopMargin(size.width()); y < size.height(); y += spacing) {
        lines.moveTo(0.0, y);
        lines.lineTo(size.width(), y);
    }
    painter.drawPath(lines);
}

void paintGrid(QPainter &painter, QSizeF size, double spacing)
{
    QPainterPath lines;
    for (double y = spacing; y < size.height(); y += spacing) {
        lines.moveTo(0.0, y);
        lines.lineTo(size.width(), y);
    }
    for (double x = spacing; x < size.width(); x += spacing) {
        lines.moveTo(x, 0.0);
        lines.lineTo(x, size.height());
    }
    painter.drawPath(lines);
}

void paintDots(QPainter &painter, QSizeF size, double spacing, const QColor &color)
{
    const double radius = std::max(kDotRadiusPt, kMinDevicePixels / deviceScale(painter));
    QPainterPath dots;
    for (double y = spacing; y < size.height(); y += spacing) {
        for (double x = spacing; x < size.width(); x += spacing)
            dots.addEllipse(QPointF(x, y), radius, radius);
    }
    painter.fillPath(dots, color);
}

} // namespace

QColor patternColor(const core::PaperStyle &style)
{
    const bool dark = core::isDarkPaper(style.paperColor);
    switch (style.background.kind) {
    case PatternKind::Ruled:
        return dark ? QColor::fromRgbF(1.0F, 1.0F, 1.0F, 0.16F)
                    : QColor::fromRgbF(30 / 255.0F, 58 / 255.0F, 138 / 255.0F, 0.17F);
    case PatternKind::Grid:
        return dark ? QColor::fromRgbF(1.0F, 1.0F, 1.0F, 0.12F)
                    : QColor::fromRgbF(24 / 255.0F, 24 / 255.0F, 27 / 255.0F, 0.10F);
    case PatternKind::Dots:
        return dark ? QColor::fromRgbF(1.0F, 1.0F, 1.0F, 0.30F)
                    : QColor::fromRgbF(24 / 255.0F, 24 / 255.0F, 27 / 255.0F, 0.28F);
    case PatternKind::Blank: break;
    }
    return Qt::transparent;
}

void paintPaper(QPainter &painter, const core::PaperStyle &style, QSizeF pageSize)
{
    painter.save();
    painter.fillRect(QRectF(QPointF(0, 0), pageSize), toQColor(style.paperColor));

    const double spacing = static_cast<double>(style.background.spacingMm) * core::kPointsPerMm;
    if (style.background.kind != PatternKind::Blank && spacing > 0.0) {
        const QColor color = patternColor(style);
        painter.setPen(
            QPen(color, std::max(kLineWidthPt, kMinDevicePixels / deviceScale(painter))));
        painter.setBrush(Qt::NoBrush);
        painter.setRenderHint(QPainter::Antialiasing, true);

        switch (style.background.kind) {
        case PatternKind::Ruled: paintRuled(painter, pageSize, spacing); break;
        case PatternKind::Grid: paintGrid(painter, pageSize, spacing); break;
        case PatternKind::Dots: paintDots(painter, pageSize, spacing, color); break;
        case PatternKind::Blank: break;
        }
    }
    painter.restore();
}

} // namespace tratto::render
