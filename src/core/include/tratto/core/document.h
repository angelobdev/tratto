// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/color.h"
#include "tratto/core/ids.h"
#include "tratto/core/paper.h"

#include <QRectF>
#include <QSizeF>
#include <QString>

#include <optional>
#include <set>
#include <variant>
#include <vector>

// The note model. All coordinates are in pt, in page space, origin top-left.
// Only commands (command.h) modify a Document.
namespace tratto::core {

struct StrokePoint
{
    float x = 0.0F;
    float y = 0.0F;
    float pressure = 0.5F; // 0..1

    friend bool operator==(StrokePoint, StrokePoint) = default;
};

enum class StrokeTool { Pen, Highlighter };

struct Stroke
{
    ObjectId id;
    StrokeTool tool = StrokeTool::Pen;
    Rgba color = kInk;
    float width = 2.0F; // base width, pt
    std::vector<StrokePoint> points; // already smoothed and simplified
    QRectF bbox; // includes the stroke width; see updateBounds()

    void updateBounds();
};

// Text boxes and images join in M3.
using Object = std::variant<Stroke>;

[[nodiscard]] ObjectId objectId(const Object &object);
[[nodiscard]] QRectF objectBounds(const Object &object);

struct Page
{
    PageId id;
    QSizeF size; // pt
    PaperStyle paper;
    std::vector<Object> objects; // drawing order

    [[nodiscard]] static Page create(const PaperStyle &paper);
    [[nodiscard]] const Object *find(ObjectId wanted) const;
};

struct Document
{
    NoteId id;
    QString title;
    std::vector<Page> pages;

    // Pages changed since the last save; commands add to it, storage clears it (M2).
    std::set<PageId> dirtyPages;

    [[nodiscard]] Page *page(PageId wanted);
    [[nodiscard]] const Page *page(PageId wanted) const;
};

} // namespace tratto::core
