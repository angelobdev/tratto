// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>

namespace tratto::core {

// 8-bit sRGB color with straight (non-premultiplied) alpha.
struct Rgba
{
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    // From 0xRRGGBB.
    static constexpr Rgba fromRgb(std::uint32_t rgb) noexcept
    {
        return { static_cast<std::uint8_t>((rgb >> 16) & 0xFF),
                 static_cast<std::uint8_t>((rgb >> 8) & 0xFF),
                 static_cast<std::uint8_t>(rgb & 0xFF), 255 };
    }

    friend constexpr bool operator==(Rgba, Rgba) noexcept = default;
};

// WCAG 2.x relative luminance, 0..1. Alpha is ignored.
[[nodiscard]] double relativeLuminance(Rgba color) noexcept;

// WCAG 2.x contrast ratio, 1..21. Alpha is ignored.
[[nodiscard]] double contrastRatio(Rgba a, Rgba b) noexcept;

// Moves each channel towards white: c + (255 - c) * amount. amount is clamped to 0..1.
[[nodiscard]] Rgba mixWithWhite(Rgba color, double amount) noexcept;

// Same color with alpha set to `alpha` (0..1, clamped).
[[nodiscard]] Rgba withAlpha(Rgba color, double alpha) noexcept;

// Minimum contrast for text, WCAG AA.
inline constexpr double kMinTextContrast = 4.5;

inline constexpr Rgba kWhite = Rgba::fromRgb(0xFFFFFF);
inline constexpr Rgba kInk = Rgba::fromRgb(0x18181B);

// Colors derived from the user-chosen accent (see docs/DESIGN.md).
struct AccentPalette
{
    Rgba accent; // fills: primary button, switches
    Rgba onAccent; // text and icons on top of `accent`
    Rgba soft; // background of the active tool / selected item
    Rgba ink; // accent used as text or icon color on the app background
};

[[nodiscard]] AccentPalette accentPalette(Rgba accent, bool dark) noexcept;

inline constexpr Rgba kDefaultAccent = Rgba::fromRgb(0x3B5BDB);

// Preset accents, in display order: blue, violet, raspberry, orange,
// green, teal, sky, graphite. Names are UI strings and live in the app.
inline constexpr std::array<Rgba, 8> kAccentPresets = {
    Rgba::fromRgb(0x3B5BDB), Rgba::fromRgb(0x7048E8), Rgba::fromRgb(0xC2255C),
    Rgba::fromRgb(0xC2410C), Rgba::fromRgb(0x237032), Rgba::fromRgb(0x0F766E),
    Rgba::fromRgb(0x1971C2), Rgba::fromRgb(0x495057),
};

} // namespace tratto::core
