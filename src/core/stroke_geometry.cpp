// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/core/stroke_geometry.h"

#include "tratto/core/tools.h"

#include <QLineF>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace tratto::core {

namespace {

// Segments per half circle in the caps.
constexpr int kCapSegments = 8;

QPointF pos(StrokePoint p)
{
    return { p.x, p.y };
}

double distanceToSegment(QPointF p, QPointF a, QPointF b, double &t)
{
    const QPointF ab = b - a;
    const double len2 = QPointF::dotProduct(ab, ab);
    t = len2 > 0.0 ? std::clamp(QPointF::dotProduct(p - a, ab) / len2, 0.0, 1.0) : 0.0;
    return QLineF(p, a + ab * t).length();
}

void rdp(const std::vector<StrokePoint> &pts, std::size_t first, std::size_t last, double tolerance,
         double pressureTolerance, std::vector<bool> &keep)
{
    if (last <= first + 1)
        return;
    double worst = 0.0;
    std::size_t worstIndex = first;
    for (std::size_t i = first + 1; i < last; ++i) {
        double t = 0.0;
        const double d = distanceToSegment(pos(pts[i]), pos(pts[first]), pos(pts[last]), t);
        const double expected
            = pts[first].pressure + (pts[last].pressure - pts[first].pressure) * t;
        const double dp = std::abs(pts[i].pressure - expected);
        // Normalized error: > 1 means the point must stay.
        const double error = std::max(d / tolerance, dp / pressureTolerance);
        if (error > worst) {
            worst = error;
            worstIndex = i;
        }
    }
    if (worst > 1.0) {
        keep[worstIndex] = true;
        rdp(pts, first, worstIndex, tolerance, pressureTolerance, keep);
        rdp(pts, worstIndex, last, tolerance, pressureTolerance, keep);
    }
}

// Half circle from `fromAngle`, with decreasing angle (y points down, so this turns
// from the left side of the stroke around its tip). Endpoints excluded.
void appendArc(std::vector<QPointF> &out, QPointF center, double radius, double fromAngle)
{
    for (int i = 1; i < kCapSegments; ++i) {
        const double a = fromAngle - std::numbers::pi * i / kCapSegments;
        out.emplace_back(center.x() + radius * std::cos(a), center.y() + radius * std::sin(a));
    }
}

} // namespace

float pressureScale(float pressure) noexcept
{
    const float p = std::clamp(pressure, 0.0F, 1.0F);
    return kMinPressureScale + (kMaxPressureScale - kMinPressureScale) * p;
}

bool farEnough(StrokePoint last, StrokePoint next, float minDistance) noexcept
{
    const float dx = next.x - last.x;
    const float dy = next.y - last.y;
    return dx * dx + dy * dy >= minDistance * minDistance;
}

std::vector<StrokePoint> smooth(const std::vector<StrokePoint> &points)
{
    if (points.size() < 3)
        return points;
    std::vector<StrokePoint> out(points);
    for (std::size_t i = 1; i + 1 < points.size(); ++i) {
        const StrokePoint &a = points[i - 1];
        const StrokePoint &b = points[i];
        const StrokePoint &c = points[i + 1];
        out[i] = { (a.x + 2 * b.x + c.x) / 4, (a.y + 2 * b.y + c.y) / 4,
                   (a.pressure + 2 * b.pressure + c.pressure) / 4 };
    }
    return out;
}

std::vector<StrokePoint> simplify(const std::vector<StrokePoint> &points, float tolerance,
                                  float pressureTolerance)
{
    if (points.size() < 3 || tolerance <= 0.0F)
        return points;
    std::vector<bool> keep(points.size(), false);
    keep.front() = keep.back() = true;
    rdp(points, 0, points.size() - 1, tolerance, std::max(pressureTolerance, 1e-4F), keep);
    std::vector<StrokePoint> out;
    for (std::size_t i = 0; i < points.size(); ++i) {
        if (keep[i])
            out.push_back(points[i]);
    }
    return out;
}

std::vector<QPointF> outline(const std::vector<StrokePoint> &points, float baseWidth,
                             bool usePressure)
{
    std::vector<QPointF> out;
    if (points.empty())
        return out;

    const auto radius = [&](const StrokePoint &p) {
        return 0.5 * baseWidth * (usePressure ? pressureScale(p.pressure) : 1.0F);
    };

    if (points.size() == 1) {
        const QPointF c = pos(points.front());
        appendArc(out, c, radius(points.front()), 0.0);
        appendArc(out, c, radius(points.front()), std::numbers::pi);
        return out;
    }

    const std::size_t n = points.size();
    std::vector<QPointF> left(n), right(n);
    std::vector<double> radii(n);
    for (std::size_t i = 0; i < n; ++i) {
        // Tangent from the neighbours; normal is the tangent turned 90°.
        const QPointF a = pos(points[i == 0 ? 0 : i - 1]);
        const QPointF b = pos(points[i + 1 == n ? n - 1 : i + 1]);
        QPointF tangent = b - a;
        const double len = std::hypot(tangent.x(), tangent.y());
        tangent = len > 0.0 ? tangent / len : QPointF(1, 0);
        const QPointF normal(-tangent.y(), tangent.x());
        radii[i] = radius(points[i]);
        left[i] = pos(points[i]) + normal * radii[i];
        right[i] = pos(points[i]) - normal * radii[i];
    }

    const auto angleOf = [](QPointF v) { return std::atan2(v.y(), v.x()); };

    out.reserve(2 * n + 2 * kCapSegments);
    out.insert(out.end(), left.begin(), left.end());
    // End cap: from the left side, around the tip, to the right side.
    appendArc(out, pos(points[n - 1]), radii[n - 1], angleOf(left[n - 1] - pos(points[n - 1])));
    for (std::size_t i = n; i-- > 0;)
        out.push_back(right[i]);
    // Start cap: from the right side, around the start, back to the left side.
    appendArc(out, pos(points[0]), radii[0], angleOf(right[0] - pos(points[0])));
    return out;
}

std::vector<StrokePoint> finishPoints(const std::vector<StrokePoint> &raw)
{
    return simplify(smooth(raw), kSimplifyTolerance);
}

} // namespace tratto::core
