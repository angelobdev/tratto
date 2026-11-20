// SPDX-License-Identifier: GPL-3.0-or-later
// The real UI (Main.qml and the whole QML module) driven with mouse, touch and
// right clicks: navigation between the library and the editor, context menus.
#include "appservices.h"
#include "canvasitem.h"

#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/QQmlExtensionPlugin>

Q_IMPORT_QML_PLUGIN(TrattoPlugin)

class TestUi : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<AppServices> m_services;
    std::unique_ptr<QQmlApplicationEngine> m_engine;
    QQuickWindow *m_window = nullptr;
    QPointingDevice *m_touch = nullptr;
    QString m_firstId;

    int pageCountOf(const QString &id) const
    {
        for (const QVariant &v : m_services->library.notes()) {
            if (v.toMap().value(QStringLiteral("id")).toString() == id)
                return v.toMap().value(QStringLiteral("pageCount")).toInt();
        }
        return -1;
    }

    QQuickItem *cardTitled(const QString &title) const
    {
        for (QQuickItem *card : cards()) {
            if (card->property("note").toMap().value(QStringLiteral("title")).toString() == title)
                return card;
        }
        return nullptr;
    }

    QObject *find(const char *name) const
    {
        if (QObject *o = m_window->findChild<QObject *>(QString::fromLatin1(name)))
            return o;
        const auto found = items(name);
        return found.isEmpty() ? nullptr : found.first();
    }

    // Visual tree search: items inside a ScrollView aren't QObject children of the window.
    static void collect(QQuickItem *item, const QString &name, QList<QQuickItem *> &out)
    {
        if (item->objectName() == name && item->isVisible())
            out.append(item);
        for (QQuickItem *child : item->childItems())
            collect(child, name, out);
    }

    QList<QQuickItem *> items(const char *name) const
    {
        QList<QQuickItem *> out;
        collect(m_window->contentItem(), QString::fromLatin1(name), out);
        return out;
    }

    QList<QQuickItem *> cards() const { return items("noteCard"); }

    // Visual check: with TRATTO_UI_DUMP=<dir> (and a GPU backend, see CLAUDE.md),
    // saves the window as <dir>/<name>.png.
    void dump(const char *name) const
    {
        const QString dir = qEnvironmentVariable("TRATTO_UI_DUMP");
        if (!dir.isEmpty()) {
            QTest::qWait(300); // let tiles render
            m_window->grabWindow().save(dir + QLatin1Char('/') + QString::fromLatin1(name)
                                        + QStringLiteral(".png"));
        }
    }

    int depth() const { return find("stack")->property("depth").toInt(); }
    bool busy() const { return find("stack")->property("busy").toBool(); }

    QPoint centerOf(QQuickItem *item) const
    {
        return item->mapToScene(QPointF(item->width() / 2, item->height() / 3)).toPoint();
    }

    void click(QQuickItem *item, Qt::MouseButton button = Qt::LeftButton)
    {
        QTest::mouseClick(m_window, button, { }, centerOf(item));
    }

    // Leaves the editor and waits for the library to be back.
    void backToLibrary()
    {
        QTRY_VERIFY(!busy()); // wait for the page transition to end
        QTRY_VERIFY(!items("editorBack").isEmpty());
        auto *back = items("editorBack").first();
        QVERIFY(back);
        click(back);
        QTRY_COMPARE(depth(), 1);
        QTRY_VERIFY(!busy());
    }

private slots:
    void initTestCase()
    {
        QQuickStyle::setStyle(QStringLiteral("Basic"));
        m_touch = QTest::createTouchDevice();
    }

    void init()
    {
        cleanup(); // in case a failed init() skipped it
        m_dir = std::make_unique<QTemporaryDir>();
        m_services = std::make_unique<AppServices>(m_dir->path());
        QTRY_VERIFY(m_services->library.ready());
        const QString group
            = m_services->library.createGroup(QStringLiteral("University"), QColor(0xAE3EC9));
        m_firstId
            = m_services->library.createNote(QStringLiteral("First"), group, 1, 6, Qt::white, 0, 0);
        m_services->library.createNote(QStringLiteral("Second"), { }, 2, 5, Qt::white, 0, 0);

        m_engine = std::make_unique<QQmlApplicationEngine>();
        m_engine->loadFromModule("Tratto", "Main");
        QVERIFY(!m_engine->rootObjects().isEmpty());
        m_window = qobject_cast<QQuickWindow *>(m_engine->rootObjects().first());
        QVERIFY(m_window);
        QVERIFY(QTest::qWaitForWindowExposed(m_window));
        QTRY_COMPARE(cards().size(), 2);
        QTRY_VERIFY(cards().first()->height() > 0);
    }

    void cleanup()
    {
        m_engine.reset(); // sessions flush; then the library finishes its queue
        m_services.reset();
        m_dir.reset();
    }

    void clickOpensAnExistingNote()
    {
        click(cards().first());
        QTRY_COMPARE(depth(), 2);
    }

    void noteCanBeReopenedAfterLeavingIt()
    {
        click(cards().first());
        QTRY_COMPARE(depth(), 2);
        backToLibrary();
        QTRY_COMPARE(cards().size(), 2);
        click(cards().first());
        QTRY_COMPARE(depth(), 2);
        backToLibrary();
        click(cards().last());
        QTRY_COMPARE(depth(), 2);
    }

    void tapOpensANote()
    {
        const QPoint p = centerOf(cards().first());
        QTest::touchEvent(m_window, m_touch).press(0, p);
        QTest::touchEvent(m_window, m_touch).release(0, p);
        QTRY_COMPARE(depth(), 2);
    }

    void rightClickOpensTheMenu()
    {
        click(cards().first(), Qt::RightButton);
        QTRY_VERIFY(find("noteMenu")->property("opened").toBool());
        QCOMPARE(depth(), 1); // the note did not open
    }

    void groupCardRightClickOpensTheGroupMenu()
    {
        QList<QQuickItem *> groups;
        QTRY_VERIFY(!(groups = items("groupCard")).isEmpty());
        click(groups.first(), Qt::RightButton);
        QTRY_VERIFY(find("groupMenu")->property("opened").toBool());
    }

    void addedPagesSurviveLeavingAndReopening()
    {
        QTRY_VERIFY(cardTitled(QStringLiteral("First")));
        click(cardTitled(QStringLiteral("First")));
        QTRY_COMPARE(depth(), 2);
        QTRY_VERIFY(!busy());
        QTRY_VERIFY(!items("addPage").isEmpty());
        click(items("addPage").first());
        click(items("addPage").first());
        backToLibrary();
        QTRY_COMPARE(pageCountOf(m_firstId), 3); // saved on leaving
        QTRY_VERIFY(cardTitled(QStringLiteral("First")));
        click(cardTitled(QStringLiteral("First")));
        QTRY_COMPARE(depth(), 2);
    }

    // Opens the first note and returns its canvas (nullptr on failure).
    CanvasItem *openEditor()
    {
        click(cards().first());
        if (!QTest::qWaitFor([&] { return depth() == 2 && !busy() && !items("canvas").isEmpty(); }))
            return nullptr;
        auto *canvas = qobject_cast<CanvasItem *>(items("canvas").first());
        return QTest::qWaitFor([&] { return canvas->session()->loaded(); }) ? canvas : nullptr;
    }

    // Mouse drag on the canvas, in canvas coordinates.
    void dragOn(CanvasItem *canvas, QPointF from, QPointF to)
    {
        const QPointF origin = canvas->mapToScene(QPointF(0, 0));
        QTest::mousePress(m_window, Qt::LeftButton, { }, (origin + from).toPoint());
        for (int i = 1; i <= 12; ++i)
            QTest::mouseMove(m_window, (origin + from + (to - from) * i / 12.0).toPoint());
        QTest::mouseRelease(m_window, Qt::LeftButton, { }, (origin + to).toPoint());
    }

    void selectionMenuRecolors()
    {
        CanvasItem *canvas = openEditor();
        QVERIFY(canvas);
        dragOn(canvas, { 300, 200 }, { 400, 200 });
        canvas->tools()->setTool(EditorTools::Tool::Select);
        canvas->tools()->setProperty("selectionMode",
                                     QVariant::fromValue(EditorTools::SelectionMode::Rectangle));
        dragOn(canvas, { 280, 180 }, { 420, 220 });
        QTRY_VERIFY(!items("selectionColor").isEmpty());
        click(items("selectionColor").first());
        QTRY_VERIFY(!items("selectionSwatch2").isEmpty()); // red
        dump("selection-colors");
        click(items("selectionSwatch2").first());
        const auto &stroke = std::get<tratto::core::Stroke>(canvas->session()->page(0)->objects[0]);
        QCOMPARE(stroke.color, tratto::core::Rgba::fromRgb(0xE03131));
        QVERIFY(canvas->hasSelection()); // still selected
    }

    void eraserHighlighterOnlyFromTheToolbar()
    {
        CanvasItem *canvas = openEditor();
        QVERIFY(canvas);
        EditorTools *tools = canvas->tools();
        dragOn(canvas, { 250, 300 }, { 450, 300 }); // ink
        tools->setTool(EditorTools::Tool::Highlighter);
        dragOn(canvas, { 250, 300 }, { 450, 300 }); // highlighter over it
        QCOMPARE(canvas->session()->page(0)->objects.size(), 2u);

        tools->setTool(EditorTools::Tool::Eraser);
        QTRY_VERIFY(!items("eraseHighlighterOnly").isEmpty());
        click(items("eraseHighlighterOnly").first()); // the real checkbox
        QVERIFY(tools->eraseHighlighterOnly());
        dragOn(canvas, { 350, 250 }, { 350, 350 });
        {
            const auto &objects = canvas->session()->page(0)->objects;
            QCOMPARE(objects.size(), 1u);
            QCOMPARE(std::get<tratto::core::Stroke>(objects[0]).tool,
                     tratto::core::StrokeTool::Pen);
        }

        // Partial mode too: the highlighter is cut, the ink stays whole.
        tools->setTool(EditorTools::Tool::Highlighter);
        dragOn(canvas, { 250, 300 }, { 450, 300 });
        tools->setTool(EditorTools::Tool::Eraser);
        tools->setProperty("eraserMode", QVariant::fromValue(EditorTools::EraserMode::Partial));
        dragOn(canvas, { 350, 250 }, { 350, 350 });
        int pens = 0, highlighters = 0;
        for (const auto &o : canvas->session()->page(0)->objects)
            (std::get<tratto::core::Stroke>(o).tool == tratto::core::StrokeTool::Pen
                 ? pens
                 : highlighters)++;
        QCOMPARE(pens, 1);
        QCOMPARE(highlighters, 2);
    }

    void undoAndRedoFromTheKeyboard()
    {
        CanvasItem *canvas = openEditor();
        QVERIFY(canvas);
        dragOn(canvas, { 300, 200 }, { 400, 200 });
        QCOMPARE(canvas->session()->page(0)->objects.size(), 1u);
        QTest::keyClick(m_window, Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(canvas->session()->page(0)->objects.size(), 0u);
        QTest::keyClick(m_window, Qt::Key_Z, Qt::ControlModifier | Qt::ShiftModifier);
        QCOMPARE(canvas->session()->page(0)->objects.size(), 1u);
    }

    void selectionMenuDeletesTheSelection()
    {
        click(cards().first());
        QTRY_COMPARE(depth(), 2);
        QTRY_VERIFY(!busy());
        QTRY_VERIFY(!items("canvas").isEmpty());
        auto *canvas = qobject_cast<CanvasItem *>(items("canvas").first());
        QVERIFY(canvas);
        QTRY_VERIFY(canvas->session()->loaded());

        const QPointF origin = canvas->mapToScene(QPointF(0, 0));
        const auto at = [&](double x, double y) { return (origin + QPointF(x, y)).toPoint(); };
        // Draw with the pen.
        QTest::mousePress(m_window, Qt::LeftButton, { }, at(300, 200));
        for (int i = 1; i <= 10; ++i)
            QTest::mouseMove(m_window, at(300 + 10 * i, 200));
        QTest::mouseRelease(m_window, Qt::LeftButton, { }, at(400, 200));
        QCOMPARE(canvas->session()->page(0)->objects.size(), 1u);

        // Rectangle selection around it, then Delete from the menu.
        canvas->tools()->setTool(EditorTools::Tool::Select);
        canvas->tools()->setProperty("selectionMode",
                                     QVariant::fromValue(EditorTools::SelectionMode::Rectangle));
        QTest::mousePress(m_window, Qt::LeftButton, { }, at(280, 180));
        for (int i = 1; i <= 10; ++i)
            QTest::mouseMove(m_window, at(280 + 14 * i, 180 + 4 * i));
        QTest::mouseRelease(m_window, Qt::LeftButton, { }, at(420, 220));
        QVERIFY(canvas->hasSelection());
        QTRY_VERIFY(!items("selectionDelete").isEmpty());
        dump("selection");
        click(items("selectionDelete").first());
        QTRY_COMPARE(canvas->session()->page(0)->objects.size(), 0u);
        QVERIFY(!canvas->hasSelection());
    }

    void pressAndHoldOpensTheMenu()
    {
        const QPoint p = centerOf(cards().first());
        QTest::touchEvent(m_window, m_touch).press(0, p);
        QTest::qWait(1200);
        QTest::touchEvent(m_window, m_touch).release(0, p);
        QTRY_VERIFY(find("noteMenu")->property("opened").toBool());
        QCOMPARE(depth(), 1);
    }
};

QTEST_MAIN(TestUi)
#include "tst_ui.moc"
