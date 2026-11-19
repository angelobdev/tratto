// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/core/paper.h"

#include <QTest>

using namespace tratto::core;

class TestPaper : public QObject
{
    Q_OBJECT

private slots:
    void pageSizesInPoints()
    {
        QCOMPARE(pageSize(PageFormat::A4, Orientation::Portrait), QSizeF(595.28, 841.89));
        QCOMPARE(pageSize(PageFormat::A4, Orientation::Landscape), QSizeF(841.89, 595.28));
        QCOMPARE(pageSize(PageFormat::Letter, Orientation::Portrait), QSizeF(612, 792));
        // Continuous ignores orientation and starts as A4 portrait.
        QCOMPARE(pageSize(PageFormat::Continuous, Orientation::Landscape), QSizeF(595.28, 841.89));
    }

    void darkPaperDetection_data()
    {
        QTest::addColumn<quint32>("rgb");
        QTest::addColumn<bool>("dark");
        QTest::newRow("white") << 0xFFFFFFu << false;
        QTest::newRow("ivory") << 0xFBF6EAu << false;
        QTest::newRow("pearl gray") << 0xF1F3F5u << false;
        QTest::newRow("sage") << 0xEEF3EAu << false;
        QTest::newRow("slate") << 0x2B2D31u << true;
    }

    void darkPaperDetection()
    {
        QFETCH(quint32, rgb);
        QFETCH(bool, dark);
        QCOMPARE(isDarkPaper(Rgba::fromRgb(rgb)), dark);
    }

    void ruledMarginScalesWithWidth()
    {
        QVERIFY(qFuzzyCompare(ruledTopMargin(680.0), 96.0));
        QVERIFY(qFuzzyCompare(ruledTopMargin(340.0), 48.0));
    }
};

QTEST_GUILESS_MAIN(TestPaper)
#include "tst_paper.moc"
