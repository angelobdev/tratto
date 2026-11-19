// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/color.h"

#include <array>

// Tool presets from docs/DESIGN.md. Names shown to the user live in the app.
namespace tratto::core {

// Base widths in pt: fine, medium, thick. Pressure scales pen strokes 0.4x–1.4x.
inline constexpr std::array<float, 3> kPenWidths = { 1.0F, 2.0F, 3.5F };
inline constexpr std::array<float, 3> kHighlighterWidths = { 8.0F, 12.0F, 18.0F };
// Radius in pt of the partial eraser: small, medium, large.
inline constexpr std::array<float, 3> kEraserRadii = { 5.0F, 8.0F, 12.0F };

inline constexpr float kMinPressureScale = 0.4F;
inline constexpr float kMaxPressureScale = 1.4F;

// Black, blue, red, green, orange, violet.
inline constexpr std::array<Rgba, 6> kPenColors = {
    Rgba::fromRgb(0x18181B), Rgba::fromRgb(0x1E3A8A), Rgba::fromRgb(0xE03131),
    Rgba::fromRgb(0x2B8A3E), Rgba::fromRgb(0xE8590C), Rgba::fromRgb(0x7048E8),
};

// Default black ink becomes this light gray on dark paper.
inline constexpr Rgba kInkOnDarkPaper = Rgba::fromRgb(0xF1F3F5);

// Yellow, green, pink, sky, orange. Drawn at kHighlighterAlpha.
inline constexpr std::array<Rgba, 5> kHighlighterColors = {
    Rgba::fromRgb(0xFFD43B), Rgba::fromRgb(0x8CE99A), Rgba::fromRgb(0xFFA8C5),
    Rgba::fromRgb(0x74C0FC), Rgba::fromRgb(0xFFC078),
};
inline constexpr double kHighlighterAlpha = 0.45;

// Text tool sizes in pt, and the default.
inline constexpr std::array<int, 5> kTextSizes = { 12, 14, 16, 20, 28 };
inline constexpr int kDefaultTextSize = 16;

} // namespace tratto::core
