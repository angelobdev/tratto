// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/document.h"

#include <QPointF>

#include <vector>

// Stroke pipeline (ARCHITECTURE §5): raw samples → close-point filter → (on release)
// smoothing → simplification → variable-width outline, filled as a polygon.
namespace tratto::core {

// Pressure 0..1 → width multiplier kMinPressureScale..kMaxPressureScale (0.4x–1.4x).
[[nodiscard]] float pressureScale(float pressure) noexcept;

// True when `next` is far enough from `last` to keep. `minDistance` is in pt;
// callers pass 0.5 screen px converted to pt at the current zoom.
[[nodiscard]] bool farEnough(StrokePoint last, StrokePoint next, float minDistance) noexcept;

// One pass of a [1 2 1] / 4 filter on position and pressure. Endpoints stay put.
[[nodiscard]] std::vector<StrokePoint> smooth(const std::vector<StrokePoint> &points);

// Ramer–Douglas–Peucker. Keeps a point when it is more than `tolerance` pt off the
// simplified line, or when its pressure differs from the interpolated one by more
// than `pressureTolerance`, so pressure swells survive.
[[nodiscard]] std::vector<StrokePoint> simplify(const std::vector<StrokePoint> &points,
                                                float tolerance, float pressureTolerance = 0.08F);

// Closed polygon around the stroke, with round caps. `usePressure` false gives a
// constant width (highlighter, mouse). A single point gives a dot.
[[nodiscard]] std::vector<QPointF> outline(const std::vector<StrokePoint> &points, float baseWidth,
                                           bool usePressure);

// Final processing on pen-up: smooth, then simplify with a low tolerance.
[[nodiscard]] std::vector<StrokePoint> finishPoints(const std::vector<StrokePoint> &raw);

// Tolerance used by finishPoints, in pt.
inline constexpr float kSimplifyTolerance = 0.15F;

} // namespace tratto::core
