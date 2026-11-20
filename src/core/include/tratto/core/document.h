// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/color.h"
#include "tratto/core/ids.h"
#include "tratto/core/paper.h"

#include <QDateTime>
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

// A note's metadata: what the library shows without opening the note.
struct NoteInfo
{
    NoteId id;
    QString title;
    std::optional<GroupId> group; // nullopt = standalone note
    QDateTime created;
    QDateTime modified;
    std::optional<QDateTime> deleted; // set = in the trash
    PaperStyle defaultPaper; // for new pages
    int pageCount = 0;

    [[nodiscard]] bool inTrash() const { return deleted.has_value(); }
};

struct Group
{
    GroupId id;
    QString name;
    Rgba color;
    int order = 0;
};

struct Document
{
    NoteInfo info;
    std::vector<Page> pages;

    // Changes not saved yet. Commands add pages to dirtyPages; storage clears both.
    std::set<PageId> dirtyPages;
    bool infoDirty = false; // title, group, page order...

    [[nodiscard]] bool isDirty() const { return infoDirty || !dirtyPages.empty(); }

    [[nodiscard]] Page *page(PageId wanted);
    [[nodiscard]] const Page *page(PageId wanted) const;
};

} // namespace tratto::core
