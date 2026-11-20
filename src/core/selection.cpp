// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/core/selection.h"

#include <algorithm>
#include <cmath>

namespace tratto::core {

Affine Affine::scaling(double sx, double sy, QPointF anchor)
{
    return { sx, 0, 0, sy, anchor.x() * (1 - sx), anchor.y() * (1 - sy) };
}

QPointF Affine::map(QPointF p) const
{
    return { m11 * p.x() + m21 * p.y() + dx, m12 * p.x() + m22 * p.y() + dy };
}

QRectF Affine::mapRect(const QRectF &r) const
{
    const QPointF corners[]
        = { map(r.topLeft()), map(r.topRight()), map(r.bottomLeft()), map(r.bottomRight()) };
    double l = corners[0].x(), t = corners[0].y(), rr = l, b = t;
    for (const QPointF &c : corners) {
        l = std::min(l, c.x());
        rr = std::max(rr, c.x());
        t = std::min(t, c.y());
        b = std::max(b, c.y());
    }
    return QRectF(QPointF(l, t), QPointF(rr, b));
}

double Affine::lengthScale() const
{
    return std::sqrt(std::abs(m11 * m22 - m12 * m21));
}

bool pointInPolygon(const std::vector<QPointF> &poly, QPointF p)
{
    int winding = 0;
    for (std::size_t i = 0; i < poly.size(); ++i) {
        const QPointF a = poly[i];
        const QPointF b = poly[(i + 1) % poly.size()];
        const double cross = (b.x() - a.x()) * (p.y() - a.y()) - (p.x() - a.x()) * (b.y() - a.y());
        if (a.y() <= p.y()) {
            if (b.y() > p.y() && cross > 0)
                ++winding;
        } else if (b.y() <= p.y() && cross < 0) {
            --winding;
        }
    }
    return winding != 0;
}

std::vector<ObjectId> selectByLasso(const Page &page, const std::vector<QPointF> &polygon,
                                    double threshold)
{
    std::vector<ObjectId> ids;
    if (polygon.size() < 3)
        return ids;
    QRectF lassoBounds;
    for (const QPointF &p : polygon)
        lassoBounds |= QRectF(p, QSizeF(0.001, 0.001));
    for (const Object &object : page.objects) {
        const auto *stroke = std::get_if<Stroke>(&object);
        if (!stroke || stroke->points.empty() || !stroke->bbox.intersects(lassoBounds))
            continue;
        const auto inside = std::ranges::count_if(stroke->points, [&](const StrokePoint &p) {
            return pointInPolygon(polygon, { p.x, p.y });
        });
        if (static_cast<double>(inside) > threshold * static_cast<double>(stroke->points.size()))
            ids.push_back(stroke->id);
    }
    return ids;
}

std::vector<ObjectId> selectByRect(const Page &page, const QRectF &rect)
{
    std::vector<ObjectId> ids;
    const QRectF r = rect.normalized();
    for (const Object &object : page.objects) {
        if (objectBounds(object).intersects(r))
            ids.push_back(objectId(object));
    }
    return ids;
}

QRectF selectionBounds(const Page &page, const std::vector<ObjectId> &ids)
{
    QRectF bounds;
    for (const Object &object : page.objects) {
        if (std::ranges::find(ids, objectId(object)) != ids.end())
            bounds |= objectBounds(object);
    }
    return bounds;
}

Object transformed(const Object &object, const Affine &t)
{
    return std::visit(
        [&](const auto &o) -> Object {
            auto copy = o;
            for (StrokePoint &p : copy.points) {
                const QPointF q = t.map({ p.x, p.y });
                p.x = static_cast<float>(q.x());
                p.y = static_cast<float>(q.y());
            }
            copy.width = static_cast<float>(copy.width * t.lengthScale());
            copy.updateBounds();
            return copy;
        },
        object);
}

Object recolored(const Object &object, Rgba color)
{
    return std::visit(
        [&](const auto &o) -> Object {
            auto copy = o;
            copy.color = color;
            return copy;
        },
        object);
}

Object withNewId(const Object &object)
{
    return std::visit(
        [](const auto &o) -> Object {
            auto copy = o;
            copy.id = ObjectId::create();
            return copy;
        },
        object);
}

} // namespace tratto::core
