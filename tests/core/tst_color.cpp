// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/core/color.h"

#include <QTest>

using namespace tratto::core;

class TestColor : public QObject
{
    Q_OBJECT

private slots:
    void fromRgbSplitsChannels()
    {
        const Rgba c = Rgba::fromRgb(0x3B5BDB);
        QCOMPARE(c, (Rgba { 0x3B, 0x5B, 0xDB, 255 }));
    }

    void contrastOfBlackAndWhiteIs21()
    {
        QVERIFY(qFuzzyCompare(contrastRatio(Rgba::fromRgb(0x000000), kWhite), 21.0));
        QVERIFY(qFuzzyCompare(contrastRatio(kWhite, kWhite), 1.0));
    }

    void contrastIsSymmetric()
    {
        const Rgba a = Rgba::fromRgb(0x3B5BDB);
        QVERIFY(qFuzzyCompare(contrastRatio(a, kWhite), contrastRatio(kWhite, a)));
    }

    void presetsAreReadableOnWhite()
    {
        for (const Rgba preset : kAccentPresets) {
            QVERIFY2(
                contrastRatio(preset, kWhite) >= kMinTextContrast,
                qPrintable(
                    QStringLiteral("preset %1,%2,%3").arg(preset.r).arg(preset.g).arg(preset.b)));
            QCOMPARE(accentPalette(preset, false).onAccent, kWhite);
        }
    }

    void lightCustomAccentGetsDarkText()
    {
        const Rgba yellow = Rgba::fromRgb(0xFFD43B);
        QCOMPARE(accentPalette(yellow, false).onAccent, kInk);
    }

    void mixWithWhiteFollowsDesignFormula()
    {
        // c + (255 - c) * 0.45, rounded: 0x3B -> 147, 0x5B -> 165, 0xDB -> 235
        const Rgba mixed = mixWithWhite(Rgba::fromRgb(0x3B5BDB), 0.45);
        QCOMPARE(mixed, (Rgba { 147, 165, 235, 255 }));
        QCOMPARE(mixWithWhite(kInk, 0.0), kInk);
        QCOMPARE(mixWithWhite(kInk, 2.0), kWhite);
    }

    void paletteDependsOnScheme()
    {
        const AccentPalette light = accentPalette(kDefaultAccent, false);
        const AccentPalette dark = accentPalette(kDefaultAccent, true);
        QCOMPARE(light.ink, kDefaultAccent);
        QCOMPARE(dark.ink, mixWithWhite(kDefaultAccent, 0.45));
        QCOMPARE(light.soft.a, std::uint8_t { 31 }); // 0.12 * 255
        QCOMPARE(dark.soft.a, std::uint8_t { 66 }); // 0.26 * 255
    }
};

QTEST_GUILESS_MAIN(TestColor)
#include "tst_color.moc"
