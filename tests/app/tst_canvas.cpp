// SPDX-License-Identifier: GPL-3.0-or-later
// Input logic of the canvas: drawing, undo, erasing, panning and pinching,
// driven with synthetic mouse and touch events (the stylus can't be simulated).
#include "canvasitem.h"
#include "editortools.h"
#include "notesession.h"
#include "preferences.h"

#include <QPointingDevice>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>

namespace {

int strokeCount(const NoteSession &session)
{
    const auto *page = session.page(0);
    return page ? static_cast<int>(page->objects.size()) : -1;
}

} // namespace

class TestCanvas : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<Preferences> m_preferences;
    std::unique_ptr<QQuickWindow> m_window;
    std::unique_ptr<NoteSession> m_session;
    std::unique_ptr<EditorTools> m_tools;
    CanvasItem *m_canvas = nullptr;
    QPointingDevice *m_touch = nullptr;

    // Lasso (or rectangle) with the mouse along the edges of `r`.
    void lasso(const QRect &r)
    {
        const QPoint corners[]
            = { r.topLeft(), r.topRight(), r.bottomRight(), r.bottomLeft(), r.topLeft() };
        QTest::mousePress(m_window.get(), Qt::LeftButton, { }, corners[0]);
        for (int c = 1; c < 5; ++c) {
            for (int i = 1; i <= 8; ++i)
                QTest::mouseMove(m_window.get(),
                                 corners[c - 1] + (corners[c] - corners[c - 1]) * i / 8);
        }
        QTest::mouseRelease(m_window.get(), Qt::LeftButton, { }, corners[4]);
    }

    const tratto::core::Stroke &strokeAt(int index) const
    {
        return std::get<tratto::core::Stroke>(
            m_session->page(0)->objects[static_cast<std::size_t>(index)]);
    }

    void drag(QPoint from, QPoint to)
    {
        QTest::mousePress(m_window.get(), Qt::LeftButton, { }, from);
        for (int i = 1; i <= 10; ++i)
            QTest::mouseMove(m_window.get(), from + (to - from) * i / 10);
        QTest::mouseRelease(m_window.get(), Qt::LeftButton, { }, to);
    }

private slots:
    void initTestCase()
    {
        m_preferences = std::make_unique<Preferences>(nullptr);
        Preferences::setInstance(m_preferences.get());
        m_touch = QTest::createTouchDevice();
    }

    void init()
    {
        m_window = std::make_unique<QQuickWindow>();
        m_window->resize(900, 700);
        m_session = std::make_unique<NoteSession>();
        m_session->createBlank(1, 6, Qt::white, 2);
        m_tools = std::make_unique<EditorTools>();
        m_canvas = new CanvasItem(m_window->contentItem());
        m_canvas->setSize(QSizeF(900, 700));
        m_canvas->setSession(m_session.get());
        m_canvas->setTools(m_tools.get());
        m_window->show();
        QVERIFY(QTest::qWaitForWindowExposed(m_window.get()));
    }

    void cleanup()
    {
        m_window.reset(); // deletes the canvas
        m_session.reset();
        m_tools.reset();
    }

    void mouseDrawsAStroke()
    {
        drag({ 300, 300 }, { 500, 350 });
        QCOMPARE(strokeCount(*m_session), 1);
        QVERIFY(m_session->canUndo());

        const auto &stroke = std::get<tratto::core::Stroke>(m_session->page(0)->objects[0]);
        QVERIFY(stroke.points.size() >= 2);
        QCOMPARE(stroke.color, tratto::core::Rgba::fromRgb(0x1E3A8A)); // default: blue
        // The stroke is on the page, in pt: inside A4.
        QVERIFY(QRectF(0, 0, 595.28, 841.89).contains(stroke.bbox));
    }

    void undoAndRedo()
    {
        drag({ 300, 300 }, { 500, 350 });
        m_session->undo();
        QCOMPARE(strokeCount(*m_session), 0);
        m_session->redo();
        QCOMPARE(strokeCount(*m_session), 1);
    }

    void eraserRemovesTouchedStrokesInOneStep()
    {
        drag({ 300, 300 }, { 500, 300 });
        drag({ 300, 400 }, { 500, 400 });
        QCOMPARE(strokeCount(*m_session), 2);

        m_tools->setTool(EditorTools::Tool::Eraser);
        drag({ 400, 250 }, { 400, 450 }); // crosses both
        QCOMPARE(strokeCount(*m_session), 0);

        m_session->undo(); // one step brings both back
        QCOMPARE(strokeCount(*m_session), 2);
    }

    void partialEraserCutsStrokesInTwo()
    {
        drag({ 250, 300 }, { 650, 300 });
        QCOMPARE(strokeCount(*m_session), 1);

        m_tools->setTool(EditorTools::Tool::Eraser);
        m_tools->setProperty("eraserMode", QVariant::fromValue(EditorTools::EraserMode::Partial));
        drag({ 450, 250 }, { 450, 350 }); // across the middle
        QCOMPARE(strokeCount(*m_session), 2);
        // The page is centered: 680 px wide in a 900 px canvas, so it starts at x = 110.
        const double eraserX = (450 - 110) * 595.28 / 680; // in pt
        const double radius = m_tools->eraserRadiusPt();
        for (const auto &object : m_session->page(0)->objects) {
            for (const auto &p : std::get<tratto::core::Stroke>(object).points)
                QVERIFY2(std::abs(p.x - eraserX) >= radius * 0.9, qPrintable(QString::number(p.x)));
        }

        m_session->undo(); // one step: the stroke is whole again
        QCOMPARE(strokeCount(*m_session), 1);
    }

    void highlighterOnlyKeepsInk()
    {
        drag({ 250, 300 }, { 650, 300 });
        m_tools->setTool(EditorTools::Tool::Highlighter);
        drag({ 250, 400 }, { 650, 400 });
        QCOMPARE(strokeCount(*m_session), 2);

        m_tools->setTool(EditorTools::Tool::Eraser);
        m_tools->setProperty("eraseHighlighterOnly", true);
        drag({ 450, 250 }, { 450, 450 });
        QCOMPARE(strokeCount(*m_session), 1);
        QCOMPARE(std::get<tratto::core::Stroke>(m_session->page(0)->objects[0]).tool,
                 tratto::core::StrokeTool::Pen);
    }

    void lassoSelectsEnclosedStrokes()
    {
        drag({ 300, 300 }, { 400, 300 });
        drag({ 300, 500 }, { 400, 500 });
        m_tools->setTool(EditorTools::Tool::Select);
        lasso(QRect(QPoint(280, 280), QPoint(420, 330)));
        QVERIFY(m_canvas->hasSelection());
        QVERIFY(m_canvas->selectionRect().contains(QPointF(350, 300)));
        QVERIFY(!m_canvas->selectionRect().contains(QPointF(350, 500)));
        QCOMPARE(strokeCount(*m_session), 2); // selecting draws nothing
        QVERIFY(!m_session->canUndo() || strokeCount(*m_session) == 2);
    }

    void rectangleModeSelectsTouchedStrokes()
    {
        drag({ 300, 300 }, { 400, 300 });
        m_tools->setTool(EditorTools::Tool::Select);
        m_tools->setProperty("selectionMode",
                             QVariant::fromValue(EditorTools::SelectionMode::Rectangle));
        drag({ 380, 280 }, { 450, 320 }); // only the end of the stroke
        QVERIFY(m_canvas->hasSelection());
    }

    void moveSelectionAndUndo()
    {
        drag({ 300, 300 }, { 400, 300 });
        const float x0 = strokeAt(0).points.front().x;
        m_tools->setTool(EditorTools::Tool::Select);
        lasso(QRect(QPoint(280, 280), QPoint(420, 320)));
        drag({ 350, 300 }, { 450, 400 }); // drag inside the selection
        QCOMPARE(strokeCount(*m_session), 1);
        const float moved = strokeAt(0).points.front().x - x0;
        QVERIFY2(std::abs(moved - 100 * 595.28F / 680) < 1.0F, qPrintable(QString::number(moved)));
        QVERIFY(m_canvas->hasSelection()); // still selected after moving

        m_session->undo();
        QCOMPARE(strokeAt(0).points.front().x, x0);
    }

    void resizeFromAHandle()
    {
        drag({ 300, 300 }, { 400, 300 });
        const double width0 = strokeAt(0).bbox.width();
        m_tools->setTool(EditorTools::Tool::Select);
        lasso(QRect(QPoint(280, 280), QPoint(420, 320)));
        const QRectF box = m_canvas->selectionRect();
        // Drag the bottom-right handle so the box doubles in size.
        drag(box.bottomRight().toPoint(),
             (box.topLeft() + (box.bottomRight() - box.topLeft()) * 2).toPoint());
        QVERIFY2(strokeAt(0).bbox.width() > width0 * 1.7,
                 qPrintable(QString::number(strokeAt(0).bbox.width())));
        QVERIFY(strokeAt(0).width > 2.5F); // width scales too
    }

    void deleteDuplicateRecolor()
    {
        drag({ 300, 300 }, { 400, 300 });
        m_tools->setTool(EditorTools::Tool::Select);
        lasso(QRect(QPoint(280, 280), QPoint(420, 320)));

        m_canvas->duplicateSelection();
        QCOMPARE(strokeCount(*m_session), 2);
        QVERIFY(strokeAt(1).id != strokeAt(0).id);
        QVERIFY(strokeAt(1).bbox.top() > strokeAt(0).bbox.top()); // offset down-right

        m_canvas->recolorSelection(QColor(0xE03131)); // the duplicate is selected
        QCOMPARE(strokeAt(1).color, tratto::core::Rgba::fromRgb(0xE03131));
        QVERIFY(strokeAt(0).color != tratto::core::Rgba::fromRgb(0xE03131));

        m_canvas->deleteSelection();
        QCOMPARE(strokeCount(*m_session), 1);
        QVERIFY(!m_canvas->hasSelection());
    }

    void copyAndPaste()
    {
        drag({ 300, 300 }, { 400, 300 });
        m_tools->setTool(EditorTools::Tool::Select);
        lasso(QRect(QPoint(280, 280), QPoint(420, 320)));
        m_canvas->copySelection();
        m_canvas->setPageIndex(1); // paste on another page
        m_canvas->paste();
        QCOMPARE(static_cast<int>(m_session->page(1)->objects.size()), 1);
        QVERIFY(m_canvas->hasSelection());
    }

    void switchingToolClearsSelection()
    {
        drag({ 300, 300 }, { 400, 300 });
        m_tools->setTool(EditorTools::Tool::Select);
        lasso(QRect(QPoint(280, 280), QPoint(420, 320)));
        QVERIFY(m_canvas->hasSelection());
        m_tools->setTool(EditorTools::Tool::Pen);
        QVERIFY(!m_canvas->hasSelection());
    }

    void nonDrawingToolsDoNothing()
    {
        m_tools->setTool(EditorTools::Tool::Text);
        drag({ 300, 300 }, { 500, 350 });
        QCOMPARE(strokeCount(*m_session), 0);
    }

    void fingerPansWithStylusOnly()
    {
        QSignalSpy view(m_canvas, &CanvasItem::viewChanged);
        QTest::touchEvent(m_window.get(), m_touch).press(0, { 400, 500 });
        QTest::touchEvent(m_window.get(), m_touch).move(0, { 400, 400 });
        QTest::touchEvent(m_window.get(), m_touch).move(0, { 400, 300 });
        QTest::touchEvent(m_window.get(), m_touch).release(0, { 400, 300 });
        QCOMPARE(strokeCount(*m_session), 0); // fingers never draw
        QVERIFY(view.count() > 0); // the page scrolled
    }

    void twoFingerTapUndoes()
    {
        drag({ 300, 300 }, { 500, 350 });
        QTest::touchEvent(m_window.get(), m_touch).press(0, { 300, 300 }).press(1, { 400, 300 });
        QTest::touchEvent(m_window.get(), m_touch)
            .release(0, { 300, 300 })
            .release(1, { 400, 300 });
        QCOMPARE(strokeCount(*m_session), 0);
    }

    void threeFingerTapRedoes()
    {
        drag({ 300, 300 }, { 500, 350 });
        m_session->undo();
        QCOMPARE(strokeCount(*m_session), 0);
        // Fingers land and lift a little apart in time and wobble, as real ones do.
        QTest::touchEvent(m_window.get(), m_touch).press(0, { 300, 300 });
        QTest::touchEvent(m_window.get(), m_touch).stationary(0).press(1, { 380, 300 });
        QTest::qWait(60);
        QTest::touchEvent(m_window.get(), m_touch)
            .stationary(0)
            .stationary(1)
            .press(2, { 460, 300 });
        QTest::touchEvent(m_window.get(), m_touch)
            .move(0, { 306, 305 })
            .move(1, { 386, 296 })
            .move(2, { 455, 306 });
        QTest::qWait(150);
        QTest::touchEvent(m_window.get(), m_touch)
            .release(0, { 306, 305 })
            .stationary(1)
            .stationary(2);
        QTest::qWait(40);
        QTest::touchEvent(m_window.get(), m_touch)
            .release(1, { 386, 296 })
            .release(2, { 455, 306 });
        QCOMPARE(strokeCount(*m_session), 1);
    }

    void pinchZooms()
    {
        QCOMPARE(m_canvas->zoom(), 1.0);
        QTest::touchEvent(m_window.get(), m_touch).press(0, { 400, 300 }).press(1, { 500, 300 });
        QTest::touchEvent(m_window.get(), m_touch).move(0, { 350, 300 }).move(1, { 550, 300 });
        QTest::touchEvent(m_window.get(), m_touch)
            .release(0, { 350, 300 })
            .release(1, { 550, 300 });
        QVERIFY(qAbs(m_canvas->zoom() - 2.0) < 0.01); // fingers twice as far apart
        QCOMPARE(strokeCount(*m_session), 0);
    }

    void zoomButtonsStepThroughLevels()
    {
        m_canvas->zoomIn();
        QCOMPARE(m_canvas->zoom(), 1.5);
        m_canvas->zoomOut();
        m_canvas->zoomOut();
        QCOMPARE(m_canvas->zoom(), 0.75);
    }

    void switchingPagesKeepsStrokesApart()
    {
        drag({ 300, 300 }, { 500, 350 });
        m_canvas->setPageIndex(1);
        drag({ 300, 300 }, { 500, 350 });
        drag({ 300, 400 }, { 500, 450 });
        QCOMPARE(strokeCount(*m_session), 1);
        QCOMPARE(static_cast<int>(m_session->page(1)->objects.size()), 2);
    }
};

QTEST_MAIN(TestCanvas)
#include "tst_canvas.moc"
