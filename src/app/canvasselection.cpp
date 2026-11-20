// SPDX-License-Identifier: GPL-3.0-or-later
// CanvasItem: the selection tool (lasso / rectangle), moving and resizing the
// selection, and the selection actions (copy, cut, duplicate, recolor, delete, paste).
#include "canvasgeometry.h"
#include "canvasitem.h"
#include "tratto/core/command.h"
#include "tratto/render/page_painter.h"
#include "tratto/storage/format.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QPainter>
#include <QSGFlatColorMaterial>

#include <algorithm>

namespace core = tratto::core;
using namespace canvasgeometry;

namespace {

// Clipboard format for objects copied in Tratto: a page (format.h) with the objects.
const QString kObjectsMime = QStringLiteral("application/x-tratto-objects");

constexpr double kHandleHitPx = 18.0; // touch-friendly hit area around a corner
constexpr double kHandleSizePx = 10.0;
constexpr double kSelectionPaddingPx = 6.0;
constexpr double kMinBoxPx = 28.0;
constexpr double kPasteOffsetPt = 16.0; // duplicates and pastes land a bit lower-right
constexpr double kMinScale = 0.05;
constexpr double kDashPx = 7.0, kGapPx = 5.0;

// Dashed polyline as triangles, in item px.
void appendDashed(std::vector<QPointF> &tris, const std::vector<QPointF> &points, bool closed)
{
    double phase = 0.0; // distance into the current dash+gap period
    const std::size_t n = points.size();
    for (std::size_t i = 0; i + 1 < n + (closed ? 1 : 0); ++i) {
        QPointF a = points[i];
        const QPointF b = points[(i + 1) % n];
        double remaining = QLineF(a, b).length();
        const QPointF dir = remaining > 0 ? (b - a) / remaining : QPointF();
        while (remaining > 0) {
            const bool inDash = phase < kDashPx;
            const double left = inDash ? kDashPx - phase : kDashPx + kGapPx - phase;
            const double step = std::min(left, remaining);
            const QPointF next = a + dir * step;
            if (inDash)
                appendSegment(tris, a, next, 0.9);
            a = next;
            remaining -= step;
            phase = std::fmod(phase + step, kDashPx + kGapPx);
        }
    }
}

} // namespace

QRectF CanvasItem::selectionRect() const
{
    if (m_selection.empty())
        return { };
    const QRectF pt = m_dragTransform.mapRect(m_selectionBounds);
    QRectF r = QRectF(m_offset + pt.topLeft() * scale(), pt.size() * scale())
                   .adjusted(-kSelectionPaddingPx, -kSelectionPaddingPx, kSelectionPaddingPx,
                             kSelectionPaddingPx);
    // Keep the handles apart on thin selections (a horizontal line).
    const double grow = std::max(0.0, kMinBoxPx - r.width()) / 2;
    const double growY = std::max(0.0, kMinBoxPx - r.height()) / 2;
    return r.adjusted(-grow, -growY, grow, growY);
}

bool CanvasItem::selectionBusy() const
{
    return m_gesture == Gesture::Selecting || m_gesture == Gesture::Moving
        || m_gesture == Gesture::Resizing;
}

bool CanvasItem::overOverlay(QPointF scenePos) const
{
    return std::ranges::any_of(m_overlays, [&](const QQuickItem *overlay) {
        return overlay && overlay->isVisible()
            && overlay->contains(overlay->mapFromScene(scenePos));
    });
}

// ---------------------------------------------------------------------------
// Selection state

void CanvasItem::setSelection(std::vector<core::ObjectId> ids)
{
    m_selection = std::move(ids);
    m_dragTransform = { };
    const core::Page *page = m_session ? m_session->page(m_pageIndex) : nullptr;
    m_selectionBounds = page ? core::selectionBounds(*page, m_selection) : QRectF();
    m_selectionDirty = true;
    update();
    emit selectionChanged();
}

void CanvasItem::clearSelection()
{
    if (m_selection.empty() && m_lassoPoints.empty())
        return;
    m_lassoPoints.clear();
    setSelection({ });
}

void CanvasItem::refreshSelection()
{
    const core::Page *page = m_session ? m_session->page(m_pageIndex) : nullptr;
    if (!page) {
        setSelection({ });
        return;
    }
    std::vector<core::ObjectId> kept;
    for (const core::ObjectId &id : m_selection) {
        if (page->find(id))
            kept.push_back(id);
    }
    setSelection(std::move(kept));
}

void CanvasItem::selectAll()
{
    const core::Page *page = m_session ? m_session->page(m_pageIndex) : nullptr;
    if (!page)
        return;
    std::vector<core::ObjectId> ids;
    for (const core::Object &o : page->objects)
        ids.push_back(core::objectId(o));
    setSelection(std::move(ids));
}

std::vector<core::Object> CanvasItem::selectedObjects() const
{
    std::vector<core::Object> objects;
    const core::Page *page = m_session ? m_session->page(m_pageIndex) : nullptr;
    if (!page)
        return objects;
    for (const core::Object &o : page->objects) {
        if (std::ranges::find(m_selection, core::objectId(o)) != m_selection.end())
            objects.push_back(o);
    }
    return objects;
}

void CanvasItem::refreshSnapshot()
{
    const core::Page *page = m_session ? m_session->page(m_pageIndex) : nullptr;
    if (!page) {
        m_snapshot.reset();
        return;
    }
    auto copy = std::make_shared<core::Page>(*page);
    if (m_lifted) {
        std::erase_if(copy->objects, [this](const core::Object &o) {
            return std::ranges::find(m_selection, core::objectId(o)) != m_selection.end();
        });
    }
    m_snapshot = std::move(copy);
}

void CanvasItem::lift()
{
    m_lifted = true;
    m_liftedTriangles.clear();
    for (const core::Object &o : selectedObjects())
        m_liftedTriangles.emplace_back(objectColor(o), objectTriangles(o));
    ++m_generation;
    refreshSnapshot();
    invalidate(m_selectionBounds);
}

// ---------------------------------------------------------------------------
// Gestures

bool CanvasItem::beginSelectionGesture(QPointF itemPos)
{
    const QPointF pt = toPage(itemPos);
    if (!m_selection.empty()) {
        const QRectF box = selectionRect();
        const QPointF corners[]
            = { box.topLeft(), box.topRight(), box.bottomRight(), box.bottomLeft() };
        for (int i = 0; i < 4; ++i) {
            if (QLineF(itemPos, corners[i]).length() <= kHandleHitPx) {
                const QRectF b = m_selectionBounds;
                const QPointF ptCorners[]
                    = { b.topLeft(), b.topRight(), b.bottomRight(), b.bottomLeft() };
                m_resizeAnchor = ptCorners[(i + 2) % 4];
                m_dragStart = ptCorners[i];
                m_gesture = Gesture::Resizing;
                lift();
                emit selectionChanged();
                return true;
            }
        }
        if (box.contains(itemPos)) {
            m_dragStart = pt;
            m_gesture = Gesture::Moving;
            lift();
            emit selectionChanged();
            return true;
        }
    }
    // Start a new lasso / rectangle.
    setSelection({ });
    m_lassoPoints = { pt };
    m_gesture = Gesture::Selecting;
    emit selectionChanged();
    return true;
}

void CanvasItem::continueSelectionGesture(QPointF itemPos)
{
    const QPointF pt = toPage(itemPos);
    switch (m_gesture) {
    case Gesture::Selecting:
        if (QLineF(m_lassoPoints.back(), pt).length() * scale() >= 2.0)
            m_lassoPoints.push_back(pt);
        break;
    case Gesture::Moving:
        m_dragTransform
            = core::Affine::translation(pt.x() - m_dragStart.x(), pt.y() - m_dragStart.y());
        break;
    case Gesture::Resizing: {
        // Proportional scale from the opposite corner: project the pointer on the diagonal.
        const QPointF diagonal = m_dragStart - m_resizeAnchor;
        const double len2 = QPointF::dotProduct(diagonal, diagonal);
        const double s = len2 > 0 ? QPointF::dotProduct(pt - m_resizeAnchor, diagonal) / len2 : 1.0;
        const double clamped = std::max(kMinScale, s);
        m_dragTransform = core::Affine::scaling(clamped, clamped, m_resizeAnchor);
        break;
    }
    default: return;
    }
    m_selectionDirty = true;
    update();
    emit selectionChanged();
}

void CanvasItem::endSelectionGesture()
{
    const Gesture gesture = m_gesture;
    m_gesture = Gesture::None;
    const core::Page *page = m_session ? m_session->page(m_pageIndex) : nullptr;

    if (gesture == Gesture::Selecting) {
        std::vector<core::ObjectId> ids;
        if (page && m_lassoPoints.size() >= 2) {
            const bool rectangle
                = m_tools && m_tools->selectionMode() == EditorTools::SelectionMode::Rectangle;
            ids = rectangle
                ? core::selectByRect(*page, QRectF(m_lassoPoints.front(), m_lassoPoints.back()))
                : core::selectByLasso(*page, m_lassoPoints);
        }
        m_lassoPoints.clear();
        setSelection(std::move(ids));
        return;
    }

    // Moving or resizing: commit as one command (same ids, so undo is exact).
    const core::Affine t = m_dragTransform;
    const bool changed = t.m11 != 1 || t.m22 != 1 || t.dx != 0 || t.dy != 0;
    if (page && changed) {
        std::vector<core::ReplaceObjects::Replacement> moves;
        for (const core::Object &o : selectedObjects()) {
            const core::Object moved = core::transformed(o, t);
            // Keep it drawn until its tiles are re-rendered.
            m_pending.push_back({ core::objectId(o), core::objectBounds(moved), 0,
                                  objectColor(moved), objectTriangles(moved) });
            moves.push_back({ core::objectId(o), { moved } });
        }
        m_lifted = false;
        m_session->push(std::make_unique<core::ReplaceObjects>(page->id, std::move(moves)));
        for (PendingStroke &p : m_pending) {
            if (p.generation == 0)
                p.generation = m_generation;
        }
        m_pendingDirty = true;
    } else {
        m_lifted = false;
        ++m_generation;
        refreshSnapshot();
        invalidate(m_selectionBounds);
    }
    m_liftedTriangles.clear();
    setSelection(m_selection); // recompute bounds, reset the transform
}

// ---------------------------------------------------------------------------
// Actions

void CanvasItem::deleteSelection()
{
    const core::Page *page = m_session ? m_session->page(m_pageIndex) : nullptr;
    if (!page || m_selection.empty())
        return;
    auto ids = m_selection;
    setSelection({ });
    m_session->push(std::make_unique<core::RemoveObjects>(page->id, std::move(ids)));
}

void CanvasItem::copySelection()
{
    const auto objects = selectedObjects();
    if (objects.empty())
        return;
    core::Page clip;
    clip.id = core::PageId::create();
    clip.objects = objects;

    auto *mime = new QMimeData;
    mime->setData(kObjectsMime, tratto::storage::encodePage(clip));

    // An image for other apps: the selection at 2x, on transparent background.
    const QRectF bounds = core::selectionBounds(clip, m_selection);
    QImage image((bounds.size() * 2).toSize().expandedTo({ 1, 1 }),
                 QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(2, 2);
    painter.translate(-bounds.topLeft());
    for (const core::Object &o : objects) {
        if (const auto *s = std::get_if<core::Stroke>(&o))
            tratto::render::paintStroke(painter, *s);
    }
    painter.end();
    mime->setImageData(image);
    QGuiApplication::clipboard()->setMimeData(mime);
}

void CanvasItem::cutSelection()
{
    copySelection();
    deleteSelection();
}

void CanvasItem::addAndSelect(std::vector<core::Object> objects)
{
    const core::Page *page = m_session ? m_session->page(m_pageIndex) : nullptr;
    if (!page || objects.empty())
        return;
    std::vector<core::ObjectId> ids;
    for (const core::Object &o : objects)
        ids.push_back(core::objectId(o));
    m_session->push(std::make_unique<core::AddObjects>(page->id, std::move(objects)));
    setSelection(std::move(ids));
}

void CanvasItem::duplicateSelection()
{
    std::vector<core::Object> copies;
    const auto offset = core::Affine::translation(kPasteOffsetPt, kPasteOffsetPt);
    for (const core::Object &o : selectedObjects())
        copies.push_back(core::transformed(core::withNewId(o), offset));
    addAndSelect(std::move(copies));
}

void CanvasItem::recolorSelection(const QColor &color)
{
    const core::Page *page = m_session ? m_session->page(m_pageIndex) : nullptr;
    if (!page || m_selection.empty())
        return;
    const QColor c = color.toRgb();
    const core::Rgba rgba { static_cast<std::uint8_t>(c.red()),
                            static_cast<std::uint8_t>(c.green()),
                            static_cast<std::uint8_t>(c.blue()), 255 };
    std::vector<core::ReplaceObjects::Replacement> changes;
    for (const core::Object &o : selectedObjects())
        changes.push_back({ core::objectId(o), { core::recolored(o, rgba) } });
    m_session->push(std::make_unique<core::ReplaceObjects>(page->id, std::move(changes)));
    setSelection(m_selection);
}

void CanvasItem::paste()
{
    const QMimeData *mime = QGuiApplication::clipboard()->mimeData();
    if (!mime || !mime->hasFormat(kObjectsMime))
        return; // images arrive with the image tool
    auto clip = tratto::storage::decodePage(mime->data(kObjectsMime));
    if (!clip)
        return;
    if (m_tools)
        m_tools->setTool(EditorTools::Tool::Select);
    std::vector<core::Object> objects;
    const auto offset = core::Affine::translation(kPasteOffsetPt, kPasteOffsetPt);
    for (const core::Object &o : clip->objects)
        objects.push_back(core::transformed(core::withNewId(o), offset));
    addAndSelect(std::move(objects));
}

// ---------------------------------------------------------------------------
// Drawing (render thread, called from updatePaintNode)

void CanvasItem::buildSelectionNodes(QSGNode *parent) const
{
    const double s = scale();
    const auto toItem = [&](QPointF pt) { return m_offset + pt * s; };

    // Lifted objects follow the drag transform.
    for (const auto &[color, triangles] : m_liftedTriangles) {
        std::vector<QPointF> moved;
        moved.reserve(triangles.size());
        for (const QPointF &p : triangles)
            moved.push_back(m_dragTransform.map(p));
        parent->appendChildNode(makeTriangleNode(moved, color, m_offset, s));
    }

    std::vector<QPointF> outline; // item px
    if (m_gesture == Gesture::Selecting && m_lassoPoints.size() >= 2) {
        const bool rectangle
            = m_tools && m_tools->selectionMode() == EditorTools::SelectionMode::Rectangle;
        std::vector<QPointF> path;
        if (rectangle) {
            const QRectF r
                = QRectF(toItem(m_lassoPoints.front()), toItem(m_lassoPoints.back())).normalized();
            path = { r.topLeft(), r.topRight(), r.bottomRight(), r.bottomLeft() };
        } else {
            for (const QPointF &p : m_lassoPoints)
                path.push_back(toItem(p));
        }
        appendDashed(outline, path, true);
    } else if (!m_selection.empty()) {
        const QRectF r = selectionRect();
        appendDashed(outline, { r.topLeft(), r.topRight(), r.bottomRight(), r.bottomLeft() }, true);
    }
    if (!outline.empty())
        parent->appendChildNode(makeTriangleNode(outline, m_accentColor, { }, 1.0));

    // Corner handles: white squares with an accent border.
    if (!m_selection.empty() && m_gesture != Gesture::Selecting && m_gesture != Gesture::Moving) {
        const QRectF r = selectionRect();
        std::vector<QPointF> borders, fills;
        for (const QPointF c : { r.topLeft(), r.topRight(), r.bottomRight(), r.bottomLeft() }) {
            const QRectF h(c.x() - kHandleSizePx / 2, c.y() - kHandleSizePx / 2, kHandleSizePx,
                           kHandleSizePx);
            const QRectF outer = h.adjusted(-2, -2, 2, 2);
            borders.insert(borders.end(),
                           { outer.topLeft(), outer.topRight(), outer.bottomLeft(),
                             outer.topRight(), outer.bottomRight(), outer.bottomLeft() });
            fills.insert(fills.end(),
                         { h.topLeft(), h.topRight(), h.bottomLeft(), h.topRight(), h.bottomRight(),
                           h.bottomLeft() });
        }
        parent->appendChildNode(makeTriangleNode(borders, m_accentColor, { }, 1.0));
        parent->appendChildNode(makeTriangleNode(fills, Qt::white, { }, 1.0));
    }
}
