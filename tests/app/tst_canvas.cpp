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

    void nonDrawingToolsDoNothing()
    {
        m_tools->setTool(EditorTools::Tool::Select);
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
