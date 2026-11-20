// SPDX-License-Identifier: GPL-3.0-or-later
#include "canvasitem.h"

#include "canvasgeometry.h"
#include "editortools.h"
#include "notesession.h"
#include "preferences.h"
#include "tratto/core/command.h"
#include "tratto/core/stroke_geometry.h"
#include "tratto/core/tools.h"
#include "tratto/render/page_painter.h"

#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QPainter>
#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGImageNode>
#include <QSGRectangleNode>
#include <QSet>
#include <QTabletEvent>
#include <QThread>
#include <QTouchEvent>
#include <QWheelEvent>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace core = tratto::core;
using namespace canvasgeometry;

namespace {

constexpr std::array<double, 7> kZoomLevels = { 0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 4.0 };
constexpr int kDefaultLevel = 2;

constexpr int kTileSize = 512; // device px
constexpr double kMinSampleDistancePx = 0.5; // ARCHITECTURE §5
constexpr double kEraserRadiusPx = 10.0; // whole-stroke eraser, screen px
constexpr qint64 kPalmGuardMs = 500; // ignore touch this long after stylus activity
// Two/three-finger taps: real fingers land and lift a little apart and wobble.
constexpr qint64 kTapMaxMs = 400;
constexpr double kTapMaxMovePx = 20.0;
constexpr int kSettleDelayMs = 150;

// View margins around the page, item px (from the design).
constexpr double kMarginTop = 32.0;
constexpr double kMarginBottom = 96.0;
constexpr double kMarginSide = 24.0;

quint64 tileKey(int level, int tx, int ty)
{
    return (static_cast<quint64>(level) << 40) | (static_cast<quint64>(tx) << 20)
        | static_cast<quint64>(ty);
}

int keyLevel(quint64 key)
{
    return static_cast<int>(key >> 40);
}

} // namespace

CanvasItem::CanvasItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton);
    setAcceptTouchEvents(true);
    setAcceptHoverEvents(true);
    setClip(true);
    m_offset = { 0, kMarginTop };
    m_clock.start();

    // Leave one core for the GUI and render threads.
    m_pool.setMaxThreadCount(std::max(1, QThread::idealThreadCount() - 1));

    m_settleTimer.setSingleShot(true);
    m_settleTimer.setInterval(kSettleDelayMs);
    connect(&m_settleTimer, &QTimer::timeout, this, &CanvasItem::settleZoom);
}

CanvasItem::~CanvasItem()
{
    // Tile jobs post results to this object: make sure none is still running.
    m_pool.clear();
    m_pool.waitForDone();
}

// ---------------------------------------------------------------------------
// Properties

void CanvasItem::setSession(NoteSession *session)
{
    if (session == m_session)
        return;
    if (m_session)
        disconnect(m_session, nullptr, this, nullptr);
    m_session = session;
    if (m_session)
        connect(m_session, &NoteSession::pageChanged, this, &CanvasItem::onPageChanged);
    resetPage();
    emit sessionChanged();
}

void CanvasItem::setTools(EditorTools *tools)
{
    if (tools == m_tools)
        return;
    if (m_tools)
        disconnect(m_tools, nullptr, this, nullptr);
    m_tools = tools;
    if (m_tools) {
        connect(m_tools, &EditorTools::changed, this, [this] {
            if (m_tools->tool() != EditorTools::Tool::Select && !selectionBusy())
                clearSelection();
        });
    }
    emit toolsChanged();
}

void CanvasItem::setPageIndex(int index)
{
    if (index == m_pageIndex)
        return;
    cancelInput();
    m_pageIndex = index;
    resetPage();
    emit pageIndexChanged();
}

bool CanvasItem::canZoomIn() const
{
    return m_zoom < kZoomLevels.back() - 1e-6;
}

bool CanvasItem::canZoomOut() const
{
    return m_zoom > kZoomLevels.front() + 1e-6;
}

void CanvasItem::zoomIn()
{
    const auto next
        = std::ranges::find_if(kZoomLevels, [this](double z) { return z > m_zoom + 1e-6; });
    if (next != kZoomLevels.end()) {
        zoomAround({ width() / 2, height() / 2 }, *next);
        settleZoom();
    }
}

void CanvasItem::zoomOut()
{
    const auto prev = std::find_if(kZoomLevels.rbegin(), kZoomLevels.rend(),
                                   [this](double z) { return z < m_zoom - 1e-6; });
    if (prev != kZoomLevels.rend()) {
        zoomAround({ width() / 2, height() / 2 }, *prev);
        settleZoom();
    }
}

// ---------------------------------------------------------------------------
// View

double CanvasItem::scale() const
{
    return m_zoom * kPixelsPerPointAt100;
}

QPointF CanvasItem::toPage(QPointF itemPos) const
{
    return (itemPos - m_offset) / scale();
}

QSizeF CanvasItem::pageSize() const
{
    return m_snapshot ? m_snapshot->size : QSizeF(595.28, 841.89);
}

void CanvasItem::setView(double zoom, QPointF offset)
{
    m_zoom = std::clamp(zoom, kZoomLevels.front(), kZoomLevels.back());
    m_offset = offset;
    clampOffset();
    m_pendingDirty = m_liveDirty = m_selectionDirty = true; // their geometry is in item px
    if (!m_selection.empty())
        emit selectionChanged(); // the menu follows the selection
    updateTiles();
    update();
    emit viewChanged();
}

void CanvasItem::zoomAround(QPointF itemAnchor, double zoom)
{
    const QPointF anchorOnPage = toPage(itemAnchor);
    const double clamped = std::clamp(zoom, kZoomLevels.front(), kZoomLevels.back());
    setView(clamped, itemAnchor - anchorOnPage * clamped * kPixelsPerPointAt100);
}

void CanvasItem::clampOffset()
{

    const QSizeF page = pageSize() * scale();
    const double w = width();
    const double h = height();
    if (page.width() + 2 * kMarginSide <= w)
        m_offset.setX((w - page.width()) / 2);
    else
        m_offset.setX(std::clamp(m_offset.x(), w - page.width() - kMarginSide, kMarginSide));
    if (page.height() + kMarginTop + kMarginBottom <= h)
        m_offset.setY(kMarginTop);
    else
        m_offset.setY(std::clamp(m_offset.y(), h - page.height() - kMarginBottom, kMarginTop));
}

void CanvasItem::resetView()
{
    m_zoom = kZoomLevels[kDefaultLevel];
    m_offset = { 0, kMarginTop };
    m_level = kDefaultLevel;
    setView(m_zoom, m_offset);
}

// Picks the raster level for the current zoom once a zoom gesture has finished.
void CanvasItem::settleZoom()
{
    m_settleTimer.stop();
    const int level = levelFor(m_zoom);
    if (level == m_level)
        return;
    // Drop an older transition, then keep the current tiles visible underneath.
    for (auto it = m_tiles.begin(); it != m_tiles.end();) {
        if (it->level == m_previousLevel) {
            if (it->node)
                m_nodesToDelete.push_back(it->node);
            it = m_tiles.erase(it);
        } else {
            ++it;
        }
    }
    m_previousLevel = m_level;
    m_level = level;
    updateTiles();
    update();
}

// ---------------------------------------------------------------------------
// Tiles

int CanvasItem::levelFor(double zoom) const
{
    // The smallest level at least as large as the zoom, so tiles are never upscaled.
    for (std::size_t i = 0; i < kZoomLevels.size(); ++i) {
        if (kZoomLevels[i] >= zoom - 1e-6)
            return static_cast<int>(i);
    }
    return static_cast<int>(kZoomLevels.size()) - 1;
}

double CanvasItem::rasterScale(int level) const
{
    const double dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    return kZoomLevels[static_cast<std::size_t>(level)] * kPixelsPerPointAt100 * dpr;
}

void CanvasItem::updateTiles()
{
    if (!m_snapshot || width() <= 0 || height() <= 0)
        return;

    const double rs = rasterScale(m_level);
    const QSizeF page = pageSize();
    const int pageWidthPx = static_cast<int>(std::ceil(page.width() * rs));
    const int pageHeightPx = static_cast<int>(std::ceil(page.height() * rs));

    // Visible page area, plus half a tile around it, in raster px.
    const QRectF visiblePt = QRectF(toPage({ 0, 0 }), toPage({ width(), height() }))
                                 .intersected(QRectF(QPointF(0, 0), page));
    const double margin = kTileSize / 2.0;
    const QRectF wanted(visiblePt.left() * rs - margin, visiblePt.top() * rs - margin,
                        visiblePt.width() * rs + 2 * margin, visiblePt.height() * rs + 2 * margin);

    const int tx0 = std::max(0, static_cast<int>(std::floor(wanted.left() / kTileSize)));
    const int ty0 = std::max(0, static_cast<int>(std::floor(wanted.top() / kTileSize)));
    const int tx1
        = std::min((pageWidthPx - 1) / kTileSize, static_cast<int>(wanted.right() / kTileSize));
    const int ty1
        = std::min((pageHeightPx - 1) / kTileSize, static_cast<int>(wanted.bottom() / kTileSize));

    QSet<quint64> keep;
    for (int ty = ty0; ty <= ty1; ++ty) {
        for (int tx = tx0; tx <= tx1; ++tx) {
            const quint64 key = tileKey(m_level, tx, ty);
            keep.insert(key);
            auto it = m_tiles.find(key);
            if (it == m_tiles.end()) {
                Tile tile;
                tile.level = m_level;
                tile.pixels = QRect(tx * kTileSize, ty * kTileSize,
                                    std::min(kTileSize, pageWidthPx - tx * kTileSize),
                                    std::min(kTileSize, pageHeightPx - ty * kTileSize));
                tile.points
                    = QRectF(QPointF(tile.pixels.topLeft()) / rs, QSizeF(tile.pixels.size()) / rs);
                tile.neededGeneration = m_generation;
                it = m_tiles.insert(key, tile);
            }
            requestTile(key, *it);
        }
    }

    // Evict current-level tiles that left the view. Keep the previous level until
    // every visible tile of the current level is ready.
    bool currentReady = true;
    for (const quint64 key : keep) {
        const Tile &t = m_tiles[key];
        currentReady = currentReady && t.renderedGeneration >= 0;
    }
    std::vector<quint64> evict;
    for (auto it = m_tiles.cbegin(); it != m_tiles.cend(); ++it) {
        const bool current = it->level == m_level;
        const bool previous = it->level == m_previousLevel;
        if ((current && !keep.contains(it.key())) || (previous && currentReady)
            || (!current && !previous))
            evict.push_back(it.key());
    }
    for (const quint64 key : evict)
        evictTile(key);
    if (currentReady)
        m_previousLevel = -1;
}

void CanvasItem::requestTile(quint64 key, Tile &tile)
{
    if (tile.requestedGeneration >= tile.neededGeneration)
        return;
    tile.requestedGeneration = m_generation;

    const std::shared_ptr<const core::Page> page = m_snapshot;
    const QRect pixels = tile.pixels;
    const QRectF points = tile.points;
    const double rs = rasterScale(tile.level);
    const int generation = m_generation;
    m_pool.start([this, page, pixels, points, rs, generation, key] {
        QImage image(pixels.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.translate(-pixels.topLeft());
        painter.scale(rs, rs);
        tratto::render::paintPage(painter, *page, points);
        painter.end();
        // Queued to the GUI thread; the destructor waits for running jobs.
        QMetaObject::invokeMethod(
            this, [this, key, generation, image] { tileRendered(key, generation, image); },
            Qt::QueuedConnection);
    });
}

void CanvasItem::tileRendered(quint64 key, int generation, QImage image)
{
    auto it = m_tiles.find(key);
    if (it == m_tiles.end() || generation <= it->renderedGeneration)
        return;
    it->image = std::move(image);
    it->renderedGeneration = generation;
    it->upload = true;
    dropFinishedPending();
    if (keyLevel(key) == m_level)
        updateTiles(); // may retire the previous level
    update();
}

void CanvasItem::evictTile(quint64 key)
{
    auto it = m_tiles.find(key);
    if (it == m_tiles.end())
        return;
    if (it->node)
        m_nodesToDelete.push_back(it->node);
    m_tiles.erase(it);
}

void CanvasItem::invalidate(QRectF bounds)
{
    for (auto it = m_tiles.begin(); it != m_tiles.end(); ++it) {
        if (bounds.isEmpty() || it->points.intersects(bounds))
            it->neededGeneration = m_generation;
    }
    updateTiles();
}

void CanvasItem::dropFinishedPending()
{
    const auto before = m_pending.size();
    std::erase_if(m_pending, [this](const PendingStroke &p) {
        for (const Tile &t : std::as_const(m_tiles)) {
            if (t.level == m_level && t.points.intersects(p.bounds)
                && t.renderedGeneration < p.generation)
                return false;
        }
        return true;
    });
    if (m_pending.size() != before)
        m_pendingDirty = true;
}

void CanvasItem::resetPage()
{
    for (const Tile &t : std::as_const(m_tiles)) {
        if (t.node)
            m_nodesToDelete.push_back(t.node);
    }
    m_tiles.clear();
    m_pending.clear();
    m_pendingDirty = true;
    m_selection.clear();
    m_lassoPoints.clear();
    m_lifted = false;
    m_liftedTriangles.clear();
    m_selectionDirty = true;
    m_previousLevel = -1;
    ++m_generation;

    refreshSnapshot();
    emit selectionChanged();
    if (!m_viewInitialized && m_snapshot && width() > 0) {
        m_viewInitialized = true;
        resetView();
    } else {
        clampOffset();
        updateTiles();
    }
    update();
}

void CanvasItem::onPageChanged(int index, QRectF bounds)
{
    if (index != m_pageIndex || !m_session)
        return;
    if (!m_snapshot) { // the note just finished loading
        resetPage();
        return;
    }
    const core::Page *page = m_session->page(index);
    if (!page) {
        resetPage();
        return;
    }
    ++m_generation;
    refreshSnapshot();
    if (!m_selection.empty() && !selectionBusy())
        refreshSelection(); // undo/redo may have removed or moved selected objects
    // Pending strokes that are gone or changed (undo of add / move) disappear at once.
    std::erase_if(m_pending, [page](const PendingStroke &p) {
        const core::Object *object = page->find(p.id);
        return !object || core::objectBounds(*object) != p.bounds;
    });
    m_pendingDirty = true;
    invalidate(bounds);
    update();
}

// ---------------------------------------------------------------------------
// Input: stylus

void CanvasItem::itemChange(ItemChange change, const ItemChangeData &data)
{
    // Qt Quick items have no tabletEvent() hook: filter tablet events at the window,
    // before Qt Quick turns them into synthesized mouse events (ARCHITECTURE §5).
    if (change == ItemSceneChange) {
        if (m_filteredWindow)
            m_filteredWindow->removeEventFilter(this);
        m_filteredWindow = data.window;
        if (m_filteredWindow)
            m_filteredWindow->installEventFilter(this);
    }
    QQuickItem::itemChange(change, data);
    // Tiles are rasterized for the screen's pixel ratio: redo them when it changes.
    if ((change == ItemSceneChange && data.window) || change == ItemDevicePixelRatioHasChanged)
        resetPage();
}

bool CanvasItem::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease: return handleTablet(static_cast<QTabletEvent *>(event));
    default: return QQuickItem::eventFilter(watched, event);
    }
}

bool CanvasItem::handleTablet(QTabletEvent *event)
{
    const QPointF pos = mapFromScene(event->scenePosition());
    const bool ours = m_gesture != Gesture::None && m_source == Source::Stylus;
    if (!ours
        && (!isVisible() || !isEnabled() || !contains(pos)
            || overOverlay(event->scenePosition()))) {
        hideCursor();
        return false; // the stylus is over other UI: let Qt deliver it as a mouse
    }
    const Preferences *prefs = Preferences::instance();
    const bool barrel = event->buttons() & (Qt::MiddleButton | Qt::RightButton);
    const bool eraserTip = event->pointerType() == QPointingDevice::PointerType::Eraser
        || (barrel && prefs && prefs->barrelButtonErases());

    m_lastStylusMs = m_clock.elapsed();

    switch (event->type()) {
    case QEvent::TabletPress: {
        // A stylus touching down ends any finger gesture (palm rejection).
        if (m_gesture != Gesture::None && m_source != Source::Stylus)
            cancelInput();
        beginInput(pos, static_cast<float>(event->pressure()), Source::Stylus, eraserTip);
        break;
    }
    case QEvent::TabletMove:
        if (ours)
            continueInput(pos, static_cast<float>(event->pressure()));
        else
            showCursor(pos, eraserTip); // hovering
        break;
    case QEvent::TabletRelease:
        if (ours)
            endInput();
        break;
    default: break;
    }
    event->accept();
    return true;
}

bool CanvasItem::stylusRecentlyActive() const
{
    return m_clock.elapsed() - m_lastStylusMs < kPalmGuardMs
        || (m_gesture != Gesture::None && m_source == Source::Stylus);
}

// ---------------------------------------------------------------------------
// Input: drawing and erasing

void CanvasItem::beginInput(QPointF itemPos, float pressure, Source source, bool forceEraser)
{
    cancelInput();
    if (!m_session || !m_tools || !m_snapshot)
        return;
    m_source = source;

    const EditorTools::Tool tool = forceEraser ? EditorTools::Tool::Eraser : m_tools->tool();
    if (tool == EditorTools::Tool::Eraser) {
        m_gesture = Gesture::Erasing;
        m_session->beginGroup();
        eraseAt(itemPos);
        return;
    }
    if (tool == EditorTools::Tool::Select) {
        beginSelectionGesture(itemPos);
        return;
    }
    if (tool != EditorTools::Tool::Pen && tool != EditorTools::Tool::Highlighter)
        return; // text and image tools arrive later in M3

    m_gesture = Gesture::Drawing;
    m_liveIsHighlighter = tool == EditorTools::Tool::Highlighter;
    const auto colors = m_liveIsHighlighter ? m_tools->highlighterColors() : m_tools->penColors();
    const int index
        = m_liveIsHighlighter ? m_tools->highlighterColorIndex() : m_tools->penColorIndex();
    m_liveColor = colors.value(index, Qt::black);
    // Black ink turns light on dark paper (DESIGN.md).
    if (!m_liveIsHighlighter && index == 0 && core::isDarkPaper(m_snapshot->paper.paperColor))
        m_liveColor
            = QColor(core::kInkOnDarkPaper.r, core::kInkOnDarkPaper.g, core::kInkOnDarkPaper.b);
    if (m_liveIsHighlighter)
        m_liveColor.setAlphaF(static_cast<float>(core::kHighlighterAlpha));

    const QPointF p = toPage(itemPos);
    m_livePoints = { { static_cast<float>(p.x()), static_cast<float>(p.y()), pressure } };
    continueInput(itemPos, pressure);
}

void CanvasItem::continueInput(QPointF itemPos, float pressure)
{
    if (selectionBusy()) {
        continueSelectionGesture(itemPos);
        return;
    }
    if (m_gesture == Gesture::Erasing) {
        eraseAt(itemPos);
        return;
    }
    if (m_gesture != Gesture::Drawing)
        return;

    const QPointF p = toPage(itemPos);
    const core::StrokePoint sample { static_cast<float>(p.x()), static_cast<float>(p.y()),
                                     pressure };
    const auto minDistance = static_cast<float>(kMinSampleDistancePx / scale());
    if (m_livePoints.size() > 1 && !core::farEnough(m_livePoints.back(), sample, minDistance)) {
        m_livePoints.back().pressure = std::max(m_livePoints.back().pressure, pressure);
    } else {
        m_livePoints.push_back(sample);
    }

    const float width = m_liveIsHighlighter ? m_tools->highlighterWidthPt() : m_tools->penWidthPt();
    std::vector<core::StrokePoint> shown = m_livePoints;
    if (m_liveIsHighlighter && m_tools->straightHighlighter() && shown.size() > 2)
        shown = { shown.front(), shown.back() };
    m_liveTriangles = strokeTriangles(shown, width, m_liveIsHighlighter);
    m_liveDirty = true;
    update();
}

void CanvasItem::endInput()
{
    if (selectionBusy()) {
        endSelectionGesture();
        return;
    }
    if (m_gesture == Gesture::Erasing) {
        m_session->endGroup();
        m_gesture = Gesture::None;
        return;
    }
    if (m_gesture != Gesture::Drawing) {
        m_gesture = Gesture::None;
        return;
    }
    m_gesture = Gesture::None;

    core::Stroke stroke;
    stroke.id = core::ObjectId::create();
    stroke.tool = m_liveIsHighlighter ? core::StrokeTool::Highlighter : core::StrokeTool::Pen;
    stroke.color = { static_cast<std::uint8_t>(m_liveColor.red()),
                     static_cast<std::uint8_t>(m_liveColor.green()),
                     static_cast<std::uint8_t>(m_liveColor.blue()), 255 };
    stroke.width = m_liveIsHighlighter ? m_tools->highlighterWidthPt() : m_tools->penWidthPt();
    if (m_liveIsHighlighter && m_tools->straightHighlighter() && m_livePoints.size() > 1)
        stroke.points = { m_livePoints.front(), m_livePoints.back() };
    else
        stroke.points = core::finishPoints(m_livePoints);
    stroke.updateBounds();

    // Keep drawing the stroke as geometry until its tiles have been re-rendered.
    PendingStroke pending;
    pending.id = stroke.id;
    pending.bounds = stroke.bbox;
    pending.color = m_liveColor;
    pending.triangles = strokeTriangles(stroke.points, stroke.width, m_liveIsHighlighter);

    const core::Page *page = m_session->page(m_pageIndex);
    if (!page) {
        cancelInput();
        return;
    }
    m_session->push(
        std::make_unique<core::AddObjects>(page->id, std::vector<core::Object> { stroke }));
    pending.generation = m_generation; // set by the pageChanged that push() emitted
    m_pending.push_back(std::move(pending));
    m_pendingDirty = true;

    m_livePoints.clear();
    m_liveTriangles.clear();
    m_liveDirty = true;
    update();
}

void CanvasItem::cancelInput()
{
    if (m_gesture == Gesture::Erasing && m_session)
        m_session->endGroup();
    if (selectionBusy()) {
        // Put a lifted selection back where it was.
        m_gesture = Gesture::None;
        m_lassoPoints.clear();
        if (m_lifted) {
            m_lifted = false;
            m_liftedTriangles.clear();
            ++m_generation;
            refreshSnapshot();
            invalidate(m_selectionBounds);
        }
        setSelection(m_selection);
    }
    m_gesture = Gesture::None;
    if (!m_livePoints.empty()) {
        m_livePoints.clear();
        m_liveTriangles.clear();
        m_liveDirty = true;
        update();
    }
}

void CanvasItem::eraseAt(QPointF itemPos)
{
    if (!m_session || !m_tools)
        return;
    const core::Page *page = m_session->page(m_pageIndex);
    if (!page)
        return;
    const QPointF center = toPage(itemPos);
    const bool partial = m_tools->eraserMode() == EditorTools::EraserMode::Partial;
    const double radius = eraserRadius();
    const bool highlighterOnly = m_tools->eraseHighlighterOnly();
    showCursor(itemPos, true);

    std::vector<core::ObjectId> hit;
    std::vector<core::ReplaceObjects::Replacement> cut;
    for (const core::Object &object : page->objects) {
        const auto *stroke = std::get_if<core::Stroke>(&object);
        if (!stroke || (highlighterOnly && stroke->tool != core::StrokeTool::Highlighter))
            continue;
        if (!core::touchesCircle(*stroke, center, radius))
            continue;
        if (!partial) {
            hit.push_back(stroke->id);
            continue;
        }
        // Partial: the stroke becomes the pieces outside the circle.
        auto pieces = core::eraseCircle(*stroke, center, radius);
        if (pieces.size() == 1 && pieces[0].id == stroke->id)
            continue; // the outline touched, the centerline didn't
        std::vector<core::Object> objects(std::make_move_iterator(pieces.begin()),
                                          std::make_move_iterator(pieces.end()));
        cut.push_back({ stroke->id, std::move(objects) });
    }
    if (!hit.empty())
        m_session->push(std::make_unique<core::RemoveObjects>(page->id, std::move(hit)));
    if (!cut.empty())
        m_session->push(std::make_unique<core::ReplaceObjects>(page->id, std::move(cut)));
}

double CanvasItem::eraserRadius() const
{
    const bool partial = m_tools && m_tools->eraserMode() == EditorTools::EraserMode::Partial;
    return partial ? m_tools->eraserRadiusPt() : kEraserRadiusPx / scale();
}

void CanvasItem::showCursor(QPointF itemPos, bool forceEraser)
{
    const bool erasing = forceEraser || m_gesture == Gesture::Erasing
        || (m_tools && m_tools->tool() == EditorTools::Tool::Eraser);
    const double radius = erasing && m_snapshot ? eraserRadius() * scale() : 0.0;
    if (radius == m_cursorRadius && itemPos == m_cursorPos)
        return;
    m_cursorPos = itemPos;
    m_cursorRadius = radius;
    m_cursorDirty = true;
    update();
}

void CanvasItem::hideCursor()
{
    if (m_cursorRadius == 0.0)
        return;
    m_cursorRadius = 0.0;
    m_cursorDirty = true;
    update();
}

void CanvasItem::hoverMoveEvent(QHoverEvent *event)
{
    if (!stylusRecentlyActive())
        showCursor(event->position(), false);
}

void CanvasItem::hoverLeaveEvent(QHoverEvent *)
{
    hideCursor();
}

// ---------------------------------------------------------------------------
// Input: mouse, wheel, touchpad

void CanvasItem::mousePressEvent(QMouseEvent *event)
{
    // Touch is handled in touchEvent(); ignore mouse events synthesized from it.
    if (event->device()->type() == QInputDevice::DeviceType::TouchScreen) {
        event->ignore();
        return;
    }
    if (event->button() == Qt::MiddleButton) {
        cancelInput();
        m_gesture = Gesture::Panning;
        m_source = Source::Mouse;
        m_panLast = event->position();
    } else {
        beginInput(event->position(), 0.5F, Source::Mouse, false);
    }
    setKeepMouseGrab(true);
    event->accept();
}

void CanvasItem::mouseMoveEvent(QMouseEvent *event)
{
    if (m_source != Source::Mouse)
        return;
    if (m_gesture == Gesture::Panning) {
        setView(m_zoom, m_offset + event->position() - m_panLast);
        m_panLast = event->position();
    } else {
        continueInput(event->position(), 0.5F);
    }
}

void CanvasItem::mouseReleaseEvent(QMouseEvent *)
{
    if (m_source != Source::Mouse)
        return;
    if (m_gesture == Gesture::Panning)
        m_gesture = Gesture::None;
    else
        endInput();
    setKeepMouseGrab(false);
}

void CanvasItem::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const double steps = event->angleDelta().y() / 120.0;
        zoomAround(event->position(), m_zoom * std::pow(1.1, steps));
        m_settleTimer.start();
    } else {
        QPointF delta = event->pixelDelta().isNull() ? QPointF(event->angleDelta()) / 120.0 * 60.0
                                                     : QPointF(event->pixelDelta());
        if (event->modifiers() & Qt::ShiftModifier)
            delta = QPointF(delta.y(), delta.x());
        setView(m_zoom, m_offset + delta);
    }
    event->accept();
}

bool CanvasItem::event(QEvent *event)
{
    // Touchpad pinch (Wayland, macOS).
    if (event->type() == QEvent::NativeGesture) {
        auto *gesture = static_cast<QNativeGestureEvent *>(event);
        if (gesture->gestureType() == Qt::ZoomNativeGesture) {
            zoomAround(gesture->position(), m_zoom * (1.0 + gesture->value()));
            m_settleTimer.start();
            return true;
        }
    }
    return QQuickItem::event(event);
}

// ---------------------------------------------------------------------------
// Input: touch

void CanvasItem::touchEvent(QTouchEvent *event)
{
    event->accept(); // never let touch become synthesized mouse input
    const auto &points = event->points();
    const qint64 now = m_clock.elapsed();

    if (event->type() == QEvent::TouchCancel) {
        if (m_source == Source::Touch)
            cancelInput();
        m_touchStart.clear();
        m_touchIgnored = false;
        return;
    }

    if (event->type() == QEvent::TouchBegin) {
        m_touchStart.clear();
        m_touchMaxPoints = 0;
        m_touchStartMs = now;
        m_touchMoved = false;
        // Palm rejection: touches that start while the stylus is near are ignored.
        m_touchIgnored = stylusRecentlyActive();
    }
    if (m_touchIgnored || stylusRecentlyActive()) {
        if (m_source == Source::Touch)
            cancelInput();
        m_touchIgnored = event->type() != QEvent::TouchEnd;
        return;
    }

    QList<QEventPoint> active;
    for (const QEventPoint &p : points) {
        if (p.state() == QEventPoint::Pressed)
            m_touchStart.insert(p.id(), p.position());
        if (p.state() != QEventPoint::Released)
            active.append(p);
        // Stationary points haven't moved by definition (and may carry no position).
        if (p.state() != QEventPoint::Stationary
            && QLineF(m_touchStart.value(p.id(), p.position()), p.position()).length()
                > kTapMaxMovePx)
            m_touchMoved = true;
    }
    m_touchMaxPoints = std::max(m_touchMaxPoints, static_cast<int>(active.size()));

    const Preferences *prefs = Preferences::instance();
    const bool stylusOnly = !prefs || prefs->stylusOnly();
    const bool toolDraws = m_tools
        && (m_tools->tool() == EditorTools::Tool::Pen
            || m_tools->tool() == EditorTools::Tool::Highlighter
            || m_tools->tool() == EditorTools::Tool::Eraser);

    if (event->type() == QEvent::TouchEnd || active.isEmpty()) {
        if (m_source == Source::Touch
            && (m_gesture == Gesture::Drawing || m_gesture == Gesture::Erasing))
            endInput();
        else if (m_source == Source::Touch)
            m_gesture = Gesture::None;
        settleZoom();
        // Two-finger tap: undo; three-finger tap: redo.
        const bool tap = !m_touchMoved && now - m_touchStartMs < kTapMaxMs;
        if (tap && prefs && prefs->tapToUndo() && m_session) {
            if (m_touchMaxPoints == 2)
                m_session->undo();
            else if (m_touchMaxPoints == 3)
                m_session->redo();
        }
        m_touchStart.clear();
        return;
    }

    if (active.size() == 1) {
        const QPointF pos = active.first().position();
        if (m_gesture == Gesture::None && !stylusOnly && toolDraws && m_touchMaxPoints == 1) {
            beginInput(pos, 0.5F, Source::Touch, false);
        } else if (m_source == Source::Touch
                   && (m_gesture == Gesture::Drawing || m_gesture == Gesture::Erasing)) {
            continueInput(pos, 0.5F);
        } else if (m_source == Source::Touch && m_gesture == Gesture::Panning) {
            setView(m_zoom, m_offset + pos - m_panLast);
            m_panLast = pos;
        } else if (m_gesture == Gesture::None || m_gesture == Gesture::Pinching) {
            m_gesture = Gesture::Panning; // also after lifting one finger of a pinch
            m_source = Source::Touch;
            m_panLast = pos;
        }
        return;
    }

    // Two or more fingers: pinch to zoom and pan with the first two.
    const QPointF a = active[0].position();
    const QPointF b = active[1].position();
    const QPointF mid = (a + b) / 2;
    const double distance = std::max(1.0, QLineF(a, b).length());
    if (m_gesture != Gesture::Pinching) {
        if (m_source == Source::Touch && m_gesture == Gesture::Drawing)
            cancelInput(); // a second finger turns a finger stroke into a pinch
        m_gesture = Gesture::Pinching;
        m_source = Source::Touch;
        m_pinchStartDistance = distance;
        m_pinchStartZoom = m_zoom;
        m_pinchPagePoint = toPage(mid);
    }
    const double zoom = std::clamp(m_pinchStartZoom * distance / m_pinchStartDistance,
                                   kZoomLevels.front(), kZoomLevels.back());
    setView(zoom, mid - m_pinchPagePoint * zoom * kPixelsPerPointAt100);
}

// ---------------------------------------------------------------------------
// Rendering

void CanvasItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (!m_viewInitialized && m_snapshot && newGeometry.width() > 0) {
        m_viewInitialized = true;
        resetView();
        return;
    }
    clampOffset();
    updateTiles();
    update();
}

QSGNode *CanvasItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    auto *root = static_cast<CanvasRoot *>(oldNode);
    if (!root) {
        // New (or rebuilt) scene graph: every node we knew about is gone.
        root = new CanvasRoot;
        m_nodesToDelete.clear();
        for (Tile &t : m_tiles) {
            t.node = nullptr;
            t.upload = !t.image.isNull();
        }
        m_pendingDirty = m_liveDirty = m_cursorDirty = m_selectionDirty = true;
    }

    for (QSGNode *node : m_nodesToDelete) {
        if (QSGNode *parent = node->parent())
            parent->removeChildNode(node);
        delete node;
    }
    m_nodesToDelete.clear();

    const double s = scale();
    const auto toItem
        = [&](const QRectF &pt) { return QRectF(m_offset + pt.topLeft() * s, pt.size() * s); };

    // Page frame: a soft shadow and a 1 px outline, in item px.
    deleteChildren(root->frame);
    if (m_snapshot) {
        const QRectF page(m_offset, pageSize() * s);
        const std::array<std::pair<double, double>, 3> shadow
            = { { { 8, 0.025 }, { 4, 0.04 }, { 1, 0.06 } } };
        for (const auto &[spread, alpha] : shadow) {
            QSGRectangleNode *r = window()->createRectangleNode();
            r->setRect(page.adjusted(-spread, -spread / 2, spread, spread * 1.5));
            QColor c(24, 24, 27); // shadow color from DESIGN.md
            c.setAlphaF(static_cast<float>(alpha));
            r->setColor(c);
            root->frame->appendChildNode(r);
        }
        QSGRectangleNode *outline = window()->createRectangleNode();
        outline->setRect(page.adjusted(-1, -1, 1, 1));
        outline->setColor(m_frameColor);
        root->frame->appendChildNode(outline);
    }

    // Tiles
    for (Tile &t : m_tiles) {
        if (t.node && !t.upload)
            t.node->setRect(toItem(t.points));
        if (!t.upload || t.image.isNull())
            continue;
        if (t.node) {
            t.node->parent()->removeChildNode(t.node);
            delete t.node;
        }
        QSGTexture *texture = window()->createTextureFromImage(t.image);
        texture->setFiltering(QSGTexture::Linear);
        t.node = window()->createImageNode();
        t.node->setOwnsTexture(true);
        t.node->setTexture(texture);
        t.node->setFiltering(QSGTexture::Linear);
        t.node->setRect(toItem(t.points));
        (t.level == m_level ? root->tiles : root->previousTiles)->appendChildNode(t.node);
        t.upload = false;
    }

    if (m_pendingDirty) {
        deleteChildren(root->pending);
        for (const PendingStroke &p : m_pending)
            root->pending->appendChildNode(makeTriangleNode(p.triangles, p.color, m_offset, s));
        m_pendingDirty = false;
    }

    if (m_selectionDirty) {
        deleteChildren(root->selection);
        buildSelectionNodes(root->selection);
        m_selectionDirty = false;
    }

    if (m_cursorDirty) {
        deleteChildren(root->cursor);
        if (m_cursorRadius > 0.0) {
            // Dark ring with a light halo, visible on any paper.
            root->cursor->appendChildNode(makeRingNode(
                m_cursorPos, m_cursorRadius, m_cursorRadius + 3.0, QColor(255, 255, 255, 230)));
            root->cursor->appendChildNode(makeRingNode(
                m_cursorPos, m_cursorRadius, m_cursorRadius + 1.5, QColor(0x3F, 0x3F, 0x46)));
        }
        m_cursorDirty = false;
    }

    if (m_liveDirty) {
        deleteChildren(root->live);
        if (!m_liveTriangles.empty())
            root->live->appendChildNode(
                makeTriangleNode(m_liveTriangles, m_liveColor, m_offset, s));
        m_liveDirty = false;
    }

    return root;
}
