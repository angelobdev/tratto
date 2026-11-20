// SPDX-License-Identifier: GPL-3.0-or-later
#include "paperthumbnail.h"

#include "tratto/render/paper_painter.h"

#include <QPainter>
#include <QPainterPath>

namespace {

tratto::core::PatternKind toKind(PaperThumbnail::Pattern pattern)
{
    switch (pattern) {
    case PaperThumbnail::Pattern::Ruled: return tratto::core::PatternKind::Ruled;
    case PaperThumbnail::Pattern::Grid: return tratto::core::PatternKind::Grid;
    case PaperThumbnail::Pattern::Dots: return tratto::core::PatternKind::Dots;
    case PaperThumbnail::Pattern::Blank: break;
    }
    return tratto::core::PatternKind::Blank;
}

} // namespace

PaperThumbnail::PaperThumbnail(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    connect(this, &PaperThumbnail::paperChanged, this, [this] { update(); });
}

void PaperThumbnail::setPattern(Pattern pattern)
{
    if (pattern == m_pattern)
        return;
    m_pattern = pattern;
    emit paperChanged();
}

void PaperThumbnail::setSpacingMm(qreal spacingMm)
{
    if (qFuzzyCompare(spacingMm, m_spacingMm))
        return;
    m_spacingMm = spacingMm;
    emit paperChanged();
}

void PaperThumbnail::setPaperColor(const QColor &color)
{
    if (color == m_paperColor)
        return;
    m_paperColor = color;
    emit paperChanged();
}

void PaperThumbnail::setRadius(qreal radius)
{
    if (qFuzzyCompare(radius, m_radius))
        return;
    m_radius = radius;
    emit paperChanged();
}

void PaperThumbnail::setViewport(const QRectF &viewport)
{
    if (viewport == m_viewport)
        return;
    m_viewport = viewport;
    emit paperChanged();
}

void PaperThumbnail::paint(QPainter *painter)
{
    namespace core = tratto::core;

    const QSizeF page = core::pageSize(core::PageFormat::A4, core::Orientation::Portrait);
    if (width() <= 0 || height() <= 0)
        return;

    const QColor rgb = m_paperColor.toRgb();
    core::PaperStyle style;
    style.background = { toKind(m_pattern), static_cast<float>(m_spacingMm) };
    style.paperColor
        = { static_cast<std::uint8_t>(rgb.red()), static_cast<std::uint8_t>(rgb.green()),
            static_cast<std::uint8_t>(rgb.blue()), 255 };

    if (m_radius > 0.0) {
        QPainterPath clip;
        clip.addRoundedRect(QRectF(0, 0, width(), height()), m_radius, m_radius);
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setClipPath(clip);
    }
    const QRectF view = m_viewport.isEmpty() ? QRectF(QPointF(0, 0), page) : m_viewport;
    painter->scale(width() / view.width(), height() / view.height());
    painter->translate(-view.topLeft());
    tratto::render::paintPaper(*painter, style, page);
}
