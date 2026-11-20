// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/core/command.h"
#include "tratto/core/selection.h"

#include <QTest>

using namespace tratto::core;

namespace {

Stroke horizontal(float x0, float x1, float y, float width = 2.0F)
{
    Stroke s;
    s.id = ObjectId::create();
    s.width = width;
    for (float x = x0; x <= x1; x += 1.0F)
        s.points.push_back({ x, y, 0.5F });
    s.updateBounds();
    return s;
}

std::vector<QPointF> square(double x0, double y0, double x1, double y1)
{
    return { { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } };
}

} // namespace

class TestSelection : public QObject
{
    Q_OBJECT

private slots:
    void affineMapsAndScales()
    {
        const Affine t = Affine::scaling(2, 2, { 10, 10 });
        QCOMPARE(t.map({ 10, 10 }), QPointF(10, 10)); // anchor stays
        QCOMPARE(t.map({ 20, 15 }), QPointF(30, 20));
        QCOMPARE(t.lengthScale(), 2.0);
        QCOMPARE(Affine::translation(5, -3).map({ 1, 1 }), QPointF(6, -2));
        QCOMPARE(t.mapRect(QRectF(10, 10, 10, 5)), QRectF(10, 10, 20, 10));
    }

    void lassoNeedsMostPoints()
    {
        Page page = Page::create({ });
        const Stroke inside = horizontal(10, 40, 20); // fully inside
        const Stroke half = horizontal(30, 90, 30); // a third inside
        const Stroke outside = horizontal(200, 240, 20);
        for (const Stroke &s : { inside, half, outside })
            page.objects.emplace_back(s);

        const auto ids = selectByLasso(page, square(0, 0, 50, 50));
        QCOMPARE(ids, std::vector { inside.id });

        // A lasso that covers most of the second stroke picks it too.
        const auto more = selectByLasso(page, square(0, 0, 80, 50));
        QCOMPARE(more.size(), 2u);
    }

    void rectangleUsesBounds()
    {
        Page page = Page::create({ });
        const Stroke a = horizontal(10, 40, 20);
        const Stroke b = horizontal(100, 140, 20);
        page.objects.emplace_back(a);
        page.objects.emplace_back(b);
        QCOMPARE(selectByRect(page, QRectF(35, 0, 10, 40)),
                 std::vector { a.id }); // touches the end
        QCOMPARE(selectByRect(page, QRectF(QPointF(150, 40), QPointF(0, 0))).size(),
                 2u); // normalized
        QVERIFY(selectionBounds(page, { a.id, b.id }).contains(QPointF(120, 20)));
    }

    void transformKeepsIdAndScalesWidth()
    {
        const Stroke s = horizontal(0, 10, 0, 2.0F);
        const auto moved = std::get<Stroke>(transformed(s, Affine::scaling(3, 3, { 0, 0 })));
        QCOMPARE(moved.id, s.id);
        QCOMPARE(moved.points.back().x, 30.0F);
        QCOMPARE(moved.width, 6.0F);
        QVERIFY(moved.bbox.contains(QPointF(30, 0)));

        const auto red = std::get<Stroke>(recolored(s, Rgba::fromRgb(0xE03131)));
        QCOMPARE(red.color, Rgba::fromRgb(0xE03131));
        QCOMPARE(red.id, s.id);
        QVERIFY(std::get<Stroke>(withNewId(s)).id != s.id);
    }

    void moveIsExactlyUndone()
    {
        Document doc;
        doc.pages.push_back(Page::create({ }));
        const PageId page = doc.pages[0].id;
        const Stroke s = horizontal(0, 10, 5);
        UndoStack stack;
        stack.push(doc, std::make_unique<AddObjects>(page, std::vector<Object> { s }));

        std::vector<ReplaceObjects::Replacement> move;
        move.push_back({ s.id, { transformed(s, Affine::translation(0.1, 0.3)) } });
        stack.push(doc, std::make_unique<ReplaceObjects>(page, std::move(move)));
        QCOMPARE(std::get<Stroke>(doc.pages[0].objects[0]).id, s.id);
        QVERIFY(std::get<Stroke>(doc.pages[0].objects[0]).points[0].y != 5.0F);

        stack.undo(doc);
        QCOMPARE(doc.pages[0].objects.size(), 1u);
        QCOMPARE(std::get<Stroke>(doc.pages[0].objects[0]).points, s.points); // bit-exact
    }
};

QTEST_GUILESS_MAIN(TestSelection)
#include "tst_selection.moc"
