// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/render/paper_painter.h"

#include <QImage>
#include <QPainter>
#include <QTest>

using namespace tratto;

namespace {

// Renders a page at 1 px per pt.
QImage renderPage(const core::PaperStyle &style, QSize size = { 200, 300 })
{
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::magenta);
    QPainter painter(&image);
    render::paintPaper(painter, style, QSizeF(size));
    return image;
}

core::PaperStyle style(core::PatternKind kind, float spacingMm, quint32 paper = 0xFFFFFF)
{
    core::PaperStyle s;
    s.background = { kind, spacingMm };
    s.paperColor = core::Rgba::fromRgb(paper);
    return s;
}

// Count of rows, below the ruled margin, that contain a non-paper pixel in column x.
int markedRows(const QImage &image, int x, QRgb paper)
{
    int rows = 0;
    for (int y = 0; y < image.height(); ++y) {
        if (image.pixel(x, y) != paper)
            ++rows;
    }
    return rows;
}

} // namespace

class TestPaperPainter : public QObject
{
    Q_OBJECT

private slots:
    void blankFillsWithPaperColor()
    {
        const QImage image = renderPage(style(core::PatternKind::Blank, 0, 0xFBF6EA));
        QCOMPARE(image.pixel(0, 0), qRgb(0xFB, 0xF6, 0xEA));
        QCOMPARE(image.pixel(199, 299), qRgb(0xFB, 0xF6, 0xEA));
    }

    void ruledLinesStartBelowMargin()
    {
        const QImage image = renderPage(style(core::PatternKind::Ruled, 6));
        const int margin = static_cast<int>(core::ruledTopMargin(200.0));
        for (int y = 0; y < margin - 1; ++y)
            QCOMPARE(image.pixel(100, y), qRgb(255, 255, 255));
        // 6 mm ≈ 17 pt: below a ~28 pt margin on a 300 pt page, about 16 lines.
        const int rows = markedRows(image, 100, qRgb(255, 255, 255));
        QVERIFY2(rows >= 14, qPrintable(QString::number(rows)));
    }

    void gridDrawsVerticalLinesToo()
    {
        const QImage image = renderPage(style(core::PatternKind::Grid, 5));
        int marked = 0;
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixel(x, 5) != qRgb(255, 255, 255))
                ++marked;
        }
        QVERIFY(marked > 0);
    }

    void darkPaperUsesLightLines()
    {
        const auto slate = style(core::PatternKind::Ruled, 6, 0x2B2D31);
        QVERIFY(render::patternColor(slate).lightness() > 200);
        const auto white = style(core::PatternKind::Ruled, 6);
        QVERIFY(render::patternColor(white).lightness() < 128);
    }

    void scaledPaintingCoversWholeTarget()
    {
        // A thumbnail: an A4 page painted into 84 x 119 px.
        QImage image(84, 119, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::magenta);
        QPainter painter(&image);
        const QSizeF a4 = core::pageSize(core::PageFormat::A4, core::Orientation::Portrait);
        painter.scale(84 / a4.width(), 119 / a4.height());
        render::paintPaper(painter, style(core::PatternKind::Dots, 5), a4);
        painter.end();
        QVERIFY(image.pixel(83, 118) != qRgb(255, 0, 255));
    }
};

QTEST_MAIN(TestPaperPainter)
#include "tst_paper_painter.moc"
