// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/document.h"

#include <QPointF>
#include <QRectF>

#include <vector>

// Selecting objects and transforming them (ARCHITECTURE §5, selection).
namespace tratto::core {

// 2D affine transform: x' = m11·x + m21·y + dx, y' = m12·x + m22·y + dy.
struct Affine
{
    double m11 = 1, m12 = 0, m21 = 0, m22 = 1, dx = 0, dy = 0;

    static Affine translation(double x, double y) { return { 1, 0, 0, 1, x, y }; }
    // Scales by (sx, sy) keeping `anchor` in place.
    static Affine scaling(double sx, double sy, QPointF anchor);

    [[nodiscard]] QPointF map(QPointF p) const;
    [[nodiscard]] QRectF mapRect(const QRectF &r) const;
    // How much lengths grow on average (for stroke widths).
    [[nodiscard]] double lengthScale() const;
};

// Non-zero winding test.
[[nodiscard]] bool pointInPolygon(const std::vector<QPointF> &polygon, QPointF p);

// Strokes with more than `threshold` of their points inside the lasso polygon, and
// text boxes whose center is inside.
[[nodiscard]] std::vector<ObjectId>
selectByLasso(const Page &page, const std::vector<QPointF> &polygon, double threshold = 0.5);
// Objects whose bounds intersect the rectangle.
[[nodiscard]] std::vector<ObjectId> selectByRect(const Page &page, const QRectF &rect);

// Union of the selected objects' bounds (pt).
[[nodiscard]] QRectF selectionBounds(const Page &page, const std::vector<ObjectId> &ids);

// Copies of objects: moved/scaled (same id), recolored (same id), or with a new id.
[[nodiscard]] Object transformed(const Object &object, const Affine &transform);
[[nodiscard]] Object recolored(const Object &object, Rgba color);
[[nodiscard]] Object withNewId(const Object &object);

} // namespace tratto::core
