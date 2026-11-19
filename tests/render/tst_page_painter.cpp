// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/render/page_painter.h"

#include <QImage>
#include <QPainter>
#include <QTest>

using namespace tratto;

namespace {

core::Stroke horizontalStroke(core::StrokeTool tool, core::Rgba color, float y, float width)
{
    core::Stroke s;
    s.id = core::ObjectId::create();
    s.tool = tool;
    s.color = color;
    s.width = width;
    for (int x = 20; x <= 180; x += 10)
        s.points.push_back({ static_cast<float>(x), y, 0.5F });
    s.updateBounds();
    return s;
}

core::Page blankPage()
{
    core::Page page;
    page.id = core::PageId::create();
    page.size = QSizeF(200, 200);
    return page;
}

QImage renderPage(const core::Page &page, const QRectF &clip = { })
{
    QImage image(200, 200, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::magenta);
    QPainter painter(&image);
    render::paintPage(painter, page, clip);
    return image;
}

} // namespace

class TestPagePainter : public QObject
{
    Q_OBJECT

private slots:
    void penStrokeIsFilledWithItsColor()
    {
        core::Page page = blankPage();
        page.objects.emplace_back(
            horizontalStroke(core::StrokeTool::Pen, core::Rgba::fromRgb(0xE03131), 100, 6));
        const QImage image = renderPage(page);
        QCOMPARE(image.pixel(100, 100), qRgb(0xE0, 0x31, 0x31));
        QCOMPARE(image.pixel(100, 120), qRgb(255, 255, 255)); // paper
    }

    void highlighterIsTranslucentAndBelowInk()
    {
        core::Page page = blankPage();
        // Ink added first, highlighter on top in drawing order: the highlighter
        // must still be painted below the ink.
        page.objects.emplace_back(horizontalStroke(core::StrokeTool::Pen, core::kInk, 100, 4));
        page.objects.emplace_back(horizontalStroke(core::StrokeTool::Highlighter,
                                                   core::Rgba::fromRgb(0xFFD43B), 100, 18));
        const QImage image = renderPage(page);
        QCOMPARE(image.pixel(100, 100), qRgb(0x18, 0x18, 0x1B)); // ink stays on top, opaque
        const QColor halo = image.pixelColor(100, 106); // highlighter only
        QVERIFY(halo.red() > 240 && halo.blue() < 200 && halo.blue() > 100);
    }

    void clipSkipsObjectsOutsideIt()
    {
        core::Page page = blankPage();
        page.objects.emplace_back(horizontalStroke(core::StrokeTool::Pen, core::kInk, 20, 4));
        page.objects.emplace_back(horizontalStroke(core::StrokeTool::Pen, core::kInk, 180, 4));
        const QImage image = renderPage(page, QRectF(0, 0, 200, 50));
        QCOMPARE(image.pixel(100, 20), qRgb(0x18, 0x18, 0x1B));
        QCOMPARE(image.pixel(100, 180), qRgb(255, 255, 255));
    }
};

QTEST_MAIN(TestPagePainter)
#include "tst_page_painter.moc"
