// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/core/command.h"

#include <QTest>

using namespace tratto::core;

namespace {

Stroke makeStroke(float x)
{
    Stroke s;
    s.id = ObjectId::create();
    s.points = { { x, 10, 0.5F }, { x + 10, 20, 0.5F } };
    s.updateBounds();
    return s;
}

Document makeDocument()
{
    Document doc;
    doc.id = NoteId::create();
    doc.pages.push_back(Page::create(PaperStyle { }));
    return doc;
}

std::vector<ObjectId> idsOn(const Page &page)
{
    std::vector<ObjectId> ids;
    for (const Object &o : page.objects)
        ids.push_back(objectId(o));
    return ids;
}

} // namespace

class TestCommand : public QObject
{
    Q_OBJECT

private slots:
    void idsAreUniqueVersion7AndOrdered()
    {
        const QUuid a = newId();
        QTest::qSleep(2);
        const QUuid b = newId();
        QVERIFY(a != b);
        QCOMPARE(a.version(), QUuid::Version(7));
        QCOMPARE(a.variant(), QUuid::DCE);
        QVERIFY(a < b); // time-ordered
    }

    void addAndUndo()
    {
        Document doc = makeDocument();
        const PageId page = doc.pages[0].id;
        UndoStack stack;
        const Stroke s = makeStroke(0);

        stack.push(doc, std::make_unique<AddObjects>(page, std::vector<Object> { s }));
        QCOMPARE(doc.pages[0].objects.size(), 1u);
        QVERIFY(doc.dirtyPages.contains(page));
        QVERIFY(stack.canUndo());
        QVERIFY(!stack.canRedo());

        QVERIFY(stack.undo(doc));
        QVERIFY(doc.pages[0].objects.empty());
        QVERIFY(stack.canRedo());

        QVERIFY(stack.redo(doc));
        QCOMPARE(doc.pages[0].objects.size(), 1u);
        QVERIFY(doc.pages[0].find(s.id));
    }

    void removeRestoresOriginalOrder()
    {
        Document doc = makeDocument();
        const PageId page = doc.pages[0].id;
        UndoStack stack;
        std::vector<Object> strokes;
        for (int i = 0; i < 5; ++i)
            strokes.emplace_back(makeStroke(static_cast<float>(i * 20)));
        stack.push(doc, std::make_unique<AddObjects>(page, strokes));
        const auto before = idsOn(doc.pages[0]);

        stack.push(doc,
                   std::make_unique<RemoveObjects>(page, std::vector { before[1], before[3] }));
        QCOMPARE(idsOn(doc.pages[0]), (std::vector { before[0], before[2], before[4] }));

        stack.undo(doc);
        QCOMPARE(idsOn(doc.pages[0]), before);

        stack.redo(doc);
        QCOMPARE(doc.pages[0].objects.size(), 3u);
    }

    void pushDiscardsRedo()
    {
        Document doc = makeDocument();
        const PageId page = doc.pages[0].id;
        UndoStack stack;
        stack.push(doc, std::make_unique<AddObjects>(page, std::vector<Object> { makeStroke(0) }));
        stack.undo(doc);
        QVERIFY(stack.canRedo());
        stack.push(doc, std::make_unique<AddObjects>(page, std::vector<Object> { makeStroke(5) }));
        QVERIFY(!stack.canRedo());
        QCOMPARE(stack.size(), 1u);
    }

    void limitDropsOldestCommands()
    {
        Document doc = makeDocument();
        const PageId page = doc.pages[0].id;
        UndoStack stack(3);
        for (int i = 0; i < 5; ++i)
            stack.push(doc,
                       std::make_unique<AddObjects>(page, std::vector<Object> { makeStroke(0) }));
        QCOMPARE(stack.size(), 3u);
        int undone = 0;
        while (stack.undo(doc))
            ++undone;
        QCOMPARE(undone, 3);
        QCOMPARE(doc.pages[0].objects.size(), 2u); // the two oldest can't be undone
    }

    void unknownPageIsIgnored()
    {
        Document doc = makeDocument();
        UndoStack stack;
        stack.push(
            doc,
            std::make_unique<AddObjects>(PageId::create(), std::vector<Object> { makeStroke(0) }));
        QVERIFY(doc.pages[0].objects.empty());
        QVERIFY(doc.dirtyPages.empty());
    }
};

QTEST_GUILESS_MAIN(TestCommand)
#include "tst_command.moc"
