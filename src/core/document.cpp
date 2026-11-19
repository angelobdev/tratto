// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/core/document.h"

#include <algorithm>

namespace tratto::core {

void Stroke::updateBounds()
{
    if (points.empty()) {
        bbox = { };
        return;
    }
    float minX = points.front().x, maxX = minX, minY = points.front().y, maxY = minY;
    for (const StrokePoint &p : points) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }
    // Widest possible outline: base width at the maximum pressure scale.
    const double margin = static_cast<double>(width) * 1.4 / 2.0;
    bbox = QRectF(QPointF(minX, minY), QPointF(maxX, maxY))
               .adjusted(-margin, -margin, margin, margin);
}

ObjectId objectId(const Object &object)
{
    return std::visit([](const auto &o) { return o.id; }, object);
}

QRectF objectBounds(const Object &object)
{
    return std::visit([](const auto &o) { return o.bbox; }, object);
}

Page Page::create(const PaperStyle &paper)
{
    return { PageId::create(), pageSize(paper.format, paper.orientation), paper, { } };
}

const Object *Page::find(ObjectId wanted) const
{
    const auto it = std::ranges::find_if(
        objects, [wanted](const Object &o) { return objectId(o) == wanted; });
    return it == objects.end() ? nullptr : &*it;
}

Page *Document::page(PageId wanted)
{
    const auto it = std::ranges::find_if(pages, [wanted](const Page &p) { return p.id == wanted; });
    return it == pages.end() ? nullptr : &*it;
}

const Page *Document::page(PageId wanted) const
{
    const auto it = std::ranges::find_if(pages, [wanted](const Page &p) { return p.id == wanted; });
    return it == pages.end() ? nullptr : &*it;
}

} // namespace tratto::core
