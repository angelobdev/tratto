// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "editortools.h"
#include "notesession.h"
#include "tratto/core/document.h"
#include "tratto/core/selection.h"

#include <QColor>
#include <QElapsedTimer>
#include <QHash>
#include <QImage>
#include <QPointer>
#include <QQuickItem>
#include <QThreadPool>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <memory>
#include <optional>
#include <vector>

class QSGImageNode;
class QSGNode;
class QTabletEvent;

// The writing surface: shows one page of a NoteSession and turns stylus, mouse and
// touch input into commands. Page content is rasterized in tiles on worker threads;
// the stroke being drawn is GPU geometry updated on every input sample.
//
// Coordinates: page content is in pt. The view maps pt to item pixels with
// `offset + pt * scale`, where scale = zoom * kPixelsPerPointAt100.
class CanvasItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(NoteSession *session READ session WRITE setSession NOTIFY sessionChanged FINAL)
    Q_PROPERTY(EditorTools *tools READ tools WRITE setTools NOTIFY toolsChanged FINAL)
    Q_PROPERTY(int pageIndex READ pageIndex WRITE setPageIndex NOTIFY pageIndexChanged FINAL)
    Q_PROPERTY(qreal zoom READ zoom NOTIFY viewChanged FINAL)
    Q_PROPERTY(bool canZoomIn READ canZoomIn NOTIFY viewChanged FINAL)
    Q_PROPERTY(bool canZoomOut READ canZoomOut NOTIFY viewChanged FINAL)
    // Page outline and shadow color (from the theme).
    Q_PROPERTY(QColor frameColor MEMBER m_frameColor NOTIFY frameColorChanged FINAL)
    // Selection outline, lasso and handles (Theme.accentInk).
    Q_PROPERTY(QColor accentColor MEMBER m_accentColor NOTIFY frameColorChanged FINAL)
    // Items drawn over the canvas (menus, zoom pill): the stylus goes to them, not the canvas.
    Q_PROPERTY(QList<QQuickItem *> overlays MEMBER m_overlays NOTIFY overlaysChanged FINAL)

    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged FINAL)
    // Selection bounds in item px, for the actions menu.
    Q_PROPERTY(QRectF selectionRect READ selectionRect NOTIFY selectionChanged FINAL)
    // True while the selection is being drawn, moved or resized (hide the menu).
    Q_PROPERTY(bool selectionBusy READ selectionBusy NOTIFY selectionChanged FINAL)

    // Text editing: the QML editor sits at textEditRect (item px; height grows with
    // the text) and starts with textEditInitial. textScale = item px per pt.
    Q_PROPERTY(bool editingText READ editingText NOTIFY textEditChanged FINAL)
    Q_PROPERTY(QRectF textEditRect READ textEditRect NOTIFY textEditChanged FINAL)
    Q_PROPERTY(QString textEditInitial READ textEditInitial NOTIFY textEditChanged FINAL)
    Q_PROPERTY(qreal textScale READ scale NOTIFY viewChanged FINAL)

public:
    explicit CanvasItem(QQuickItem *parent = nullptr);
    ~CanvasItem() override;

    NoteSession *session() const { return m_session; }
    void setSession(NoteSession *session);
    EditorTools *tools() const { return m_tools; }
    void setTools(EditorTools *tools);
    int pageIndex() const { return m_pageIndex; }
    void setPageIndex(int index);

    qreal zoom() const { return m_zoom; }
    bool canZoomIn() const;
    bool canZoomOut() const;

    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();

    bool hasSelection() const { return !m_selection.empty(); }
    QRectF selectionRect() const;
    bool selectionBusy() const;

    // Selection actions (menu and shortcuts).
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void deleteSelection();
    Q_INVOKABLE void copySelection();
    Q_INVOKABLE void cutSelection();
    Q_INVOKABLE void duplicateSelection();
    Q_INVOKABLE void recolorSelection(const QColor &color);
    // Pastes objects copied in Tratto (from any note) and selects them.
    Q_INVOKABLE void paste();

    bool editingText() const { return m_editing.has_value(); }
    QRectF textEditRect() const;
    QString textEditInitial() const { return m_editing ? m_editing->text : QString(); }
    // Ends text editing: adds, updates or (when empty) removes the text box.
    Q_INVOKABLE void commitText(const QString &text);

signals:
    void sessionChanged();
    void toolsChanged();
    void pageIndexChanged();
    void viewChanged();
    void frameColorChanged();
    void overlaysChanged();
    void selectionChanged();
    void textEditChanged(); // editing state or editor geometry
    void textEditStarted(); // a box was opened: load textEditInitial into the editor
    // The QML editor must commit now (the canvas was tapped while editing).
    void textCommitRequested();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool event(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void touchEvent(QTouchEvent *event) override;

private:
    enum class Source { Stylus, Mouse, Touch };
    enum class Gesture { None, Drawing, Erasing, Panning, Pinching, Selecting, Moving, Resizing };

    struct Tile
    {
        int level = 0;
        QRect pixels; // in the page raster at this level
        QRectF points; // the same area in page pt
        QImage image;
        int renderedGeneration = -1;
        int neededGeneration = 0;
        int requestedGeneration = -1;
        bool upload = false;
        QSGImageNode *node = nullptr; // owned by the scene graph
    };

    // A committed stroke still drawn as geometry until its tiles are re-rendered.
    struct PendingStroke
    {
        tratto::core::ObjectId id;
        QRectF bounds;
        int generation = 0;
        QColor color;
        std::vector<QPointF> triangles;
    };

    // Input
    bool handleTablet(QTabletEvent *event);
    void beginInput(QPointF itemPos, float pressure, Source source, bool forceEraser);
    void continueInput(QPointF itemPos, float pressure);
    void endInput();
    void cancelInput();
    void eraseAt(QPointF itemPos);
    double eraserRadius() const; // pt

    // Selection (canvasselection.cpp)
    bool beginSelectionGesture(QPointF itemPos);
    void continueSelectionGesture(QPointF itemPos);
    void endSelectionGesture();
    void setSelection(std::vector<tratto::core::ObjectId> ids);
    void refreshSelection(); // after the page changed: drop missing ids, recompute bounds
    void lift(); // hide the selection from the tiles while it moves
    std::vector<tratto::core::Object> selectedObjects() const;
    void addAndSelect(std::vector<tratto::core::Object> objects);
    bool overOverlay(QPointF scenePos) const;
    void buildSelectionNodes(QSGNode *parent) const;
    void refreshSnapshot(); // page from the session, minus lifted / edited objects

    // Text (canvastext.cpp)
    void beginText(QPointF itemPos);
    void endTextEditing();
    // The eraser ring follows the pointer while erasing is what a press would do.
    void showCursor(QPointF itemPos, bool forceEraser);
    void hideCursor();
    bool stylusRecentlyActive() const;

    // View
    double scale() const; // item px per pt
    QPointF toPage(QPointF itemPos) const;
    QSizeF pageSize() const;
    void setView(double zoom, QPointF offset);
    void zoomAround(QPointF itemAnchor, double zoom);
    void clampOffset();
    void resetView();
    void settleZoom();

    // Tiles
    int levelFor(double zoom) const;
    double rasterScale(int level) const; // device px per pt
    void updateTiles();
    void requestTile(quint64 key, Tile &tile);
    void tileRendered(quint64 key, int generation, QImage image);
    void evictTile(quint64 key);
    void invalidate(QRectF bounds);
    void dropFinishedPending();
    void resetPage();
    void onPageChanged(int index, QRectF bounds);

    QPointer<NoteSession> m_session;
    QPointer<EditorTools> m_tools;
    int m_pageIndex = 0;
    std::shared_ptr<const tratto::core::Page> m_snapshot; // read by tile jobs
    int m_generation = 0;

    // View state
    double m_zoom = 1.0;
    QPointF m_offset;
    bool m_viewInitialized = false;
    QColor m_frameColor = QColor(24, 24, 27, 30);
    QTimer m_settleTimer;

    // Tiles: key = level << 40 | tx << 20 | ty
    QHash<quint64, Tile> m_tiles;
    int m_level = 2; // raster level being rendered (index into the zoom levels)
    int m_previousLevel = -1; // shown underneath while m_level fills in
    QThreadPool m_pool;
    std::vector<QSGNode *> m_nodesToDelete;

    // Strokes
    std::vector<PendingStroke> m_pending;
    bool m_pendingDirty = false;
    std::vector<tratto::core::StrokePoint> m_livePoints;
    std::vector<QPointF> m_liveTriangles;
    QColor m_liveColor;
    bool m_liveDirty = false;
    bool m_liveIsHighlighter = false;

    // Selection, in page pt
    std::vector<tratto::core::ObjectId> m_selection;
    QRectF m_selectionBounds;
    std::vector<QPointF> m_lassoPoints;
    bool m_lifted = false;
    std::vector<std::pair<QColor, std::vector<QPointF>>> m_liftedTriangles; // pt, untransformed
    std::vector<std::pair<QImage, QRectF>> m_liftedImages; // text boxes: image, bbox in pt

    // The text box being edited (its original stays out of the tiles meanwhile).
    std::optional<tratto::core::TextBox> m_editing;
    bool m_editingIsNew = false;
    tratto::core::Affine m_dragTransform;
    QPointF m_dragStart; // pt
    QPointF m_resizeAnchor; // pt, the corner opposite the dragged handle
    bool m_selectionDirty = false;
    QColor m_accentColor = QColor(0x3B, 0x5B, 0xDB);
    QList<QQuickItem *> m_overlays;

    // Eraser cursor (item px)
    QPointF m_cursorPos;
    double m_cursorRadius = 0.0; // px; 0 = hidden
    bool m_cursorDirty = false;

    // Gestures
    Gesture m_gesture = Gesture::None;
    Source m_source = Source::Mouse;
    QElapsedTimer m_clock;
    qint64 m_lastStylusMs = -1'000'000;
    QPointF m_panLast;
    // Touch
    QHash<int, QPointF> m_touchStart;
    int m_touchMaxPoints = 0;
    qint64 m_touchStartMs = 0;
    bool m_touchMoved = false;
    bool m_touchIgnored = false;
    double m_pinchStartDistance = 0.0;
    double m_pinchStartZoom = 1.0;
    QPointF m_pinchPagePoint; // page pt under the fingers' midpoint at pinch start

    QObject *m_filteredWindow = nullptr;
};
