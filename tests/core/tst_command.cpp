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
    doc.info.id = NoteId::create();
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

    void groupUndoesAsOneStep()
    {
        Document doc = makeDocument();
        const PageId page = doc.pages[0].id;
        UndoStack stack;
        std::vector<Object> strokes;
        for (int i = 0; i < 3; ++i)
            strokes.emplace_back(makeStroke(static_cast<float>(i * 20)));
        stack.push(doc, std::make_unique<AddObjects>(page, strokes));
        const auto ids = idsOn(doc.pages[0]);

        stack.beginGroup();
        QVERIFY(!stack.canUndo()); // no undo in the middle of a gesture
        for (const ObjectId id : ids)
            stack.push(doc, std::make_unique<RemoveObjects>(page, std::vector { id }));
        QVERIFY(doc.pages[0].objects.empty()); // applied immediately
        stack.endGroup();

        QCOMPARE(stack.size(), 2u);
        const auto change = stack.undo(doc);
        QVERIFY(change.has_value());
        QCOMPARE(change->page, page);
        QCOMPARE(idsOn(doc.pages[0]), ids);
        QVERIFY(change->bounds.contains(objectBounds(doc.pages[0].objects[2])));
    }

    void emptyGroupLeavesNoStep()
    {
        Document doc = makeDocument();
        UndoStack stack;
        stack.beginGroup();
        stack.endGroup();
        QCOMPARE(stack.size(), 0u);
        QVERIFY(!stack.canUndo());
    }

    void replaceKeepsDrawingOrderAndUndoes()
    {
        Document doc = makeDocument();
        const PageId page = doc.pages[0].id;
        UndoStack stack;
        std::vector<Object> strokes;
        for (int i = 0; i < 4; ++i)
            strokes.emplace_back(makeStroke(static_cast<float>(i * 20)));
        stack.push(doc, std::make_unique<AddObjects>(page, strokes));
        const auto before = idsOn(doc.pages[0]); // A B C D

        const Stroke b1 = makeStroke(100), b2 = makeStroke(110);
        std::vector<ReplaceObjects::Replacement> r;
        r.push_back({ before[1], { b1, b2 } }); // B → B1 B2
        r.push_back({ before[2], { } }); // C erased
        r.push_back({ before[0], { } }); // A erased
        stack.push(doc, std::make_unique<ReplaceObjects>(page, std::move(r)));
        QCOMPARE(idsOn(doc.pages[0]), (std::vector { b1.id, b2.id, before[3] }));

        stack.undo(doc);
        QCOMPARE(idsOn(doc.pages[0]), before);
        stack.redo(doc);
        QCOMPARE(idsOn(doc.pages[0]), (std::vector { b1.id, b2.id, before[3] }));
    }

    void addPageAndUndo()
    {
        Document doc = makeDocument();
        UndoStack stack;
        const Page page = Page::create(PaperStyle { });
        stack.push(doc, std::make_unique<AddPage>(page, 99)); // clamped: appended
        QCOMPARE(doc.pages.size(), 2u);
        QCOMPARE(doc.pages[1].id, page.id);
        QVERIFY(doc.infoDirty);
        QVERIFY(doc.dirtyPages.contains(page.id));
        stack.undo(doc);
        QCOMPARE(doc.pages.size(), 1u);
        QVERIFY(!doc.dirtyPages.contains(page.id));
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
