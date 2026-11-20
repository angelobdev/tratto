// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/core/stroke_geometry.h"

#include <QTest>

#include <cmath>

using namespace tratto::core;

namespace {

std::vector<StrokePoint> line(int count, float pressure = 0.5F)
{
    std::vector<StrokePoint> pts;
    for (int i = 0; i < count; ++i)
        pts.push_back({ static_cast<float>(i), 0.0F, pressure });
    return pts;
}

// Non-zero winding test (core tests can't use QtGui's QPolygonF).
bool inside(const std::vector<QPointF> &poly, QPointF p)
{
    int winding = 0;
    for (std::size_t i = 0; i < poly.size(); ++i) {
        const QPointF a = poly[i];
        const QPointF b = poly[(i + 1) % poly.size()];
        const double cross = (b.x() - a.x()) * (p.y() - a.y()) - (p.x() - a.x()) * (b.y() - a.y());
        if (a.y() <= p.y()) {
            if (b.y() > p.y() && cross > 0)
                ++winding;
        } else if (b.y() <= p.y() && cross < 0) {
            --winding;
        }
    }
    return winding != 0;
}

} // namespace

class TestStrokeGeometry : public QObject
{
    Q_OBJECT

private slots:
    void pressureScaleRange()
    {
        QCOMPARE(pressureScale(0.0F), 0.4F);
        QCOMPARE(pressureScale(1.0F), 1.4F);
        QCOMPARE(pressureScale(0.5F), 0.9F);
        QCOMPARE(pressureScale(-3.0F), 0.4F);
        QCOMPARE(pressureScale(7.0F), 1.4F);
    }

    void closePointsAreFiltered()
    {
        QVERIFY(!farEnough({ 0, 0, 0.5F }, { 0.2F, 0.2F, 0.5F }, 0.5F));
        QVERIFY(farEnough({ 0, 0, 0.5F }, { 0.5F, 0, 0.5F }, 0.5F));
    }

    void smoothingKeepsEndpointsAndCount()
    {
        std::vector<StrokePoint> zigzag;
        for (int i = 0; i < 10; ++i)
            zigzag.push_back({ static_cast<float>(i), (i % 2) ? 1.0F : -1.0F, 0.5F });
        const auto out = smooth(zigzag);
        QCOMPARE(out.size(), zigzag.size());
        QCOMPARE(out.front(), zigzag.front());
        QCOMPARE(out.back(), zigzag.back());
        // A ±1 zigzag of period 2 is cancelled: (1 - 2 + 1) / 4 = 0.
        QCOMPARE(out[4].y + 1.0F, 1.0F);
    }

    void straightLineSimplifiesToEndpoints()
    {
        const auto out = simplify(line(50), 0.15F);
        QCOMPARE(out.size(), 2u);
        QCOMPARE(out.front().x, 0.0F);
        QCOMPARE(out.back().x, 49.0F);
    }

    void cornersSurviveSimplification()
    {
        std::vector<StrokePoint> pts;
        for (int i = 0; i <= 20; ++i)
            pts.push_back({ static_cast<float>(i), 0.0F, 0.5F });
        for (int i = 1; i <= 20; ++i)
            pts.push_back({ 20.0F, static_cast<float>(i), 0.5F });
        const auto out = simplify(pts, 0.15F);
        QCOMPARE(out.size(), 3u);
        QCOMPARE(out[1].x, 20.0F);
        QCOMPARE(out[1].y, 0.0F);
    }

    void pressureChangesSurviveSimplification()
    {
        // Straight line whose pressure swells in the middle.
        auto pts = line(41);
        for (std::size_t i = 0; i < pts.size(); ++i)
            pts[i].pressure = 0.2F + 0.6F * std::sin(static_cast<float>(i) / 40.0F * 3.14159F);
        const auto out = simplify(pts, 0.15F);
        QVERIFY(out.size() > 2);
    }

    void outlineWidthFollowsPressure()
    {
        // Horizontal stroke, 4 pt base width, full pressure: 1.4x → half width 2.8 pt.
        const auto out = outline(line(21, 1.0F), 4.0F, true);
        QVERIFY(inside(out, { 10, 2.6 }));
        QVERIFY(!inside(out, { 10, 3.0 }));

        // Light pressure: 0.4x → half width 0.8 pt.
        const auto light = outline(line(21, 0.0F), 4.0F, true);
        QVERIFY(inside(light, { 10, 0.7 }));
        QVERIFY(!inside(light, { 10, 1.0 }));
    }

    void constantWidthIgnoresPressure()
    {
        const auto out = outline(line(21, 0.0F), 4.0F, false);
        QVERIFY(inside(out, { 10, 1.9 }));
        QVERIFY(!inside(out, { 10, 2.2 }));
    }

    void outlineHasRoundCapsAtBothEnds()
    {
        const auto out = outline(line(21, 0.5F), 4.0F, false); // half width 2
        QVERIFY(inside(out, { 21.8, 0 })); // beyond the end, inside the cap
        QVERIFY(inside(out, { -1.8, 0 })); // before the start, inside the cap
        QVERIFY(!inside(out, { 22.3, 0 }));
        QVERIFY(!inside(out, { -2.3, 0 }));
    }

    void singlePointIsADot()
    {
        const auto out = outline({ { 5, 5, 0.5F } }, 2.0F, false);
        QVERIFY(out.size() > 8);
        QVERIFY(inside(out, { 5, 5 }));
        QVERIFY(inside(out, { 5.9, 5 }));
        QVERIFY(!inside(out, { 6.2, 5 }));
    }

    void eraserHitTest()
    {
        Stroke s;
        s.width = 2.0F; // half width 1 at constant width
        s.tool = StrokeTool::Highlighter;
        s.points = line(21);
        s.updateBounds();
        QVERIFY(touchesCircle(s, { 10, 2.5 }, 2.0)); // 2.5 <= 2 + 1
        QVERIFY(!touchesCircle(s, { 10, 3.5 }, 2.0));
        QVERIFY(touchesCircle(s, { 22.5, 0 }, 2.0)); // past the end, within reach
        QVERIFY(!touchesCircle(s, { 40, 0 }, 2.0)); // rejected by the bounding box
    }

    void ribbonMatchesPressure()
    {
        const StrokeRibbon r = ribbon(line(3, 1.0F), 2.0F, true);
        QCOMPARE(r.left.size(), 3u);
        QVERIFY(std::abs(r.radii[1] - 1.4) < 1e-5);
        QVERIFY(std::abs(r.left[1].y() - r.right[1].y() - 2.8) < 1e-5);
    }

    void boundsIncludeMaximumWidth()
    {
        Stroke s;
        s.width = 2.0F;
        s.points = line(11, 1.0F);
        s.updateBounds();
        QCOMPARE(s.bbox, QRectF(QPointF(-1.4, -1.4), QPointF(11.4, 1.4)));
    }
};

QTEST_GUILESS_MAIN(TestStrokeGeometry)
#include "tst_stroke_geometry.moc"
