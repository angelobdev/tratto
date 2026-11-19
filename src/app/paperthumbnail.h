// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QColor>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

// A page's paper (color + pattern) scaled to the item size, drawn with the same
// renderer used for the canvas and for export.
class PaperThumbnail : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Pattern pattern READ pattern WRITE setPattern NOTIFY paperChanged FINAL)
    Q_PROPERTY(qreal spacingMm READ spacingMm WRITE setSpacingMm NOTIFY paperChanged FINAL)
    Q_PROPERTY(QColor paperColor READ paperColor WRITE setPaperColor NOTIFY paperChanged FINAL)
    // Corner radius in item pixels; the paper is clipped to it.
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY paperChanged FINAL)

public:
    enum class Pattern { Blank, Ruled, Grid, Dots };
    Q_ENUM(Pattern)

    explicit PaperThumbnail(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;

    Pattern pattern() const { return m_pattern; }
    void setPattern(Pattern pattern);
    qreal spacingMm() const { return m_spacingMm; }
    void setSpacingMm(qreal spacingMm);
    QColor paperColor() const { return m_paperColor; }
    void setPaperColor(const QColor &color);
    qreal radius() const { return m_radius; }
    void setRadius(qreal radius);

signals:
    void paperChanged();

private:
    Pattern m_pattern = Pattern::Blank;
    qreal m_spacingMm = 0.0;
    QColor m_paperColor = Qt::white;
    qreal m_radius = 0.0;
};
