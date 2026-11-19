// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/color.h"

#include <QSizeF>

namespace tratto::core {

inline constexpr double kPointsPerMm = 72.0 / 25.4;

enum class PatternKind { Blank, Ruled, Grid, Dots };

struct Background
{
    PatternKind kind = PatternKind::Blank;
    float spacingMm = 0.0F; // ignored for Blank

    friend constexpr bool operator==(Background, Background) noexcept = default;
};

enum class PageFormat { A4, Letter, Continuous };
enum class Orientation { Portrait, Landscape };

struct PaperStyle
{
    Background background;
    Rgba paperColor = kWhite;
    PageFormat format = PageFormat::A4;
    Orientation orientation = Orientation::Portrait; // ignored for Continuous

    friend constexpr bool operator==(const PaperStyle &, const PaperStyle &) noexcept = default;
};

// Page size in pt. Continuous pages start as A4 portrait and grow with their content.
[[nodiscard]] QSizeF pageSize(PageFormat format, Orientation orientation) noexcept;

// Dark paper gets light pattern lines and light default ink (docs/DESIGN.md).
[[nodiscard]] bool isDarkPaper(Rgba paperColor) noexcept;

// Blank space above the first ruled line, in pt, for a page `pageWidth` pt wide.
[[nodiscard]] double ruledTopMargin(double pageWidth) noexcept;

} // namespace tratto::core
