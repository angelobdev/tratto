// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/core/color.h"

#include <algorithm>
#include <cmath>

namespace tratto::core {

namespace {

constexpr double kAccentOnDarkMix = 0.45;
constexpr double kSoftAlphaLight = 0.12;
constexpr double kSoftAlphaDark = 0.26;

double linearize(std::uint8_t channel) noexcept
{
    const double c = channel / 255.0;
    return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
}

std::uint8_t toChannel(double value) noexcept
{
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0, 255.0)));
}

} // namespace

double relativeLuminance(Rgba color) noexcept
{
    return 0.2126 * linearize(color.r) + 0.7152 * linearize(color.g) + 0.0722 * linearize(color.b);
}

double contrastRatio(Rgba a, Rgba b) noexcept
{
    const double la = relativeLuminance(a);
    const double lb = relativeLuminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

Rgba mixWithWhite(Rgba color, double amount) noexcept
{
    const double t = std::clamp(amount, 0.0, 1.0);
    const auto mix = [t](std::uint8_t c) { return toChannel(c + (255.0 - c) * t); };
    return { mix(color.r), mix(color.g), mix(color.b), color.a };
}

Rgba withAlpha(Rgba color, double alpha) noexcept
{
    color.a = toChannel(std::clamp(alpha, 0.0, 1.0) * 255.0);
    return color;
}

AccentPalette accentPalette(Rgba accent, bool dark) noexcept
{
    accent.a = 255;
    return {
        .accent = accent,
        .onAccent = contrastRatio(accent, kWhite) >= kMinTextContrast ? kWhite : kInk,
        .soft = withAlpha(accent, dark ? kSoftAlphaDark : kSoftAlphaLight),
        .ink = dark ? mixWithWhite(accent, kAccentOnDarkMix) : accent,
    };
}

} // namespace tratto::core
