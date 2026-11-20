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

// Left and right edges of a stroke (one point each per input point) and the radius
// at each point. Used for the outline and for drawing the live stroke on the GPU.
struct StrokeRibbon
{
    std::vector<QPointF> left;
    std::vector<QPointF> right;
    std::vector<double> radii;
};
[[nodiscard]] StrokeRibbon ribbon(const std::vector<StrokePoint> &points, float baseWidth,
                                  bool usePressure);

// Closed polygon around the stroke, with round caps. `usePressure` false gives a
// constant width (highlighter, mouse). A single point gives a dot.
[[nodiscard]] std::vector<QPointF> outline(const std::vector<StrokePoint> &points, float baseWidth,
                                           bool usePressure);

// True when a circle (eraser) touches the stroke's outline.
[[nodiscard]] bool touchesCircle(const Stroke &stroke, QPointF center, double radius);

// Partial eraser: the parts of `stroke` whose centerline stays outside the circle,
// as new strokes (new ids, same style). Returns {stroke} unchanged when the circle
// doesn't reach the centerline, and nothing when the whole stroke is inside.
[[nodiscard]] std::vector<Stroke> eraseCircle(const Stroke &stroke, QPointF center, double radius);

// Final processing on pen-up: smooth, then simplify with a low tolerance.
[[nodiscard]] std::vector<StrokePoint> finishPoints(const std::vector<StrokePoint> &raw);

// Tolerance used by finishPoints, in pt.
inline constexpr float kSimplifyTolerance = 0.15F;

} // namespace tratto::core
