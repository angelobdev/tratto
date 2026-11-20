// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/render/page_painter.h"
#include "tratto/render/text_painter.h"

#include <QImage>
#include <QPainter>
#include <QTest>

using namespace tratto;

namespace {

core::TextBox box(const QString &text, double width = 200)
{
    core::TextBox t;
    t.id = core::ObjectId::create();
    t.bbox = QRectF(20, 20, width, 0);
    t.text = text;
    t.size = 16;
    return t;
}

int inkPixels(const QImage &image, const QRect &area)
{
    int count = 0;
    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            if (qGray(image.pixel(x, y)) < 128)
                ++count;
        }
    }
    return count;
}

} // namespace

class TestTextPainter : public QObject
{
    Q_OBJECT

private slots:
    void heightGrowsWithLines()
    {
        const double one = render::textHeight(box(QStringLiteral("Hello")));
        QVERIFY(one > 16 && one < 30); // about one line of 16 pt
        QCOMPARE(render::textHeight(box(QString())), one); // empty box: one line
        const double two = render::textHeight(box(QStringLiteral("Hello\nworld")));
        QVERIFY(qAbs(two - 2 * one) < 0.5);
        // Wrapping: a long sentence in a narrow box takes several lines.
        const double wrapped = render::textHeight(
            box(QStringLiteral("The quick brown fox jumps over the lazy dog"), 60));
        QVERIFY(wrapped > 3 * one - 0.5);
    }

    void sizeScalesTheHeight()
    {
        core::TextBox big = box(QStringLiteral("Hello"));
        big.size = 32;
        QVERIFY(qAbs(render::textHeight(big) - 2 * render::textHeight(box(QStringLiteral("Hello"))))
                < 0.5);
    }

    void textIsPaintedAboveInk()
    {
        core::Page page;
        page.id = core::PageId::create();
        page.size = QSizeF(240, 120);
        core::TextBox t = box(QStringLiteral("Tratto"));
        t.bbox.setHeight(render::textHeight(t));
        page.objects.emplace_back(t);

        QImage image(240, 120, QImage::Format_ARGB32_Premultiplied);
        QPainter painter(&image);
        render::paintPage(painter, page);
        painter.end();
        QVERIFY(inkPixels(image, t.bbox.toAlignedRect()) > 20); // glyphs drawn inside the box
        QCOMPARE(inkPixels(image, QRect(0, 80, 240, 40)), 0); // nothing below it
    }
};

QTEST_MAIN(TestTextPainter)
#include "tst_text_painter.moc"
