// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/core/paper.h"

namespace tratto::core {

namespace {

constexpr QSizeF kA4 { 595.28, 841.89 };
constexpr QSizeF kLetter { 612.0, 792.0 };

// Perceived brightness (0..255) below which paper counts as dark.
constexpr double kDarkPaperThreshold = 110.0;

// The design leaves ~96 px blank above the ruling on a 680 px wide page.
constexpr double kRuledTopMarginRatio = 96.0 / 680.0;

} // namespace

QSizeF pageSize(PageFormat format, Orientation orientation) noexcept
{
    switch (format) {
    case PageFormat::Continuous: return kA4;
    case PageFormat::A4: return orientation == Orientation::Portrait ? kA4 : kA4.transposed();
    case PageFormat::Letter:
        return orientation == Orientation::Portrait ? kLetter : kLetter.transposed();
    }
    return kA4;
}

bool isDarkPaper(Rgba paperColor) noexcept
{
    const double brightness = 0.299 * paperColor.r + 0.587 * paperColor.g + 0.114 * paperColor.b;
    return brightness < kDarkPaperThreshold;
}

double ruledTopMargin(double pageWidth) noexcept
{
    return pageWidth * kRuledTopMarginRatio;
}

} // namespace tratto::core
