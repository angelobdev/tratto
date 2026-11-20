// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Scene graph geometry shared by the canvas source files (internal).

#include "tratto/core/document.h"
#include "tratto/core/selection.h"
#include "tratto/core/stroke_geometry.h"

#include <QColor>
#include <QPointF>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGNode>

#include <cmath>
#include <numbers>
#include <vector>

namespace canvasgeometry {

namespace core = tratto::core;

// 100% zoom shows an A4 page 680 px wide, as in the design.
inline constexpr double kPixelsPerPointAt100 = 680.0 / 595.28;

inline constexpr int kCircleSegments = 12;

inline void appendCircle(std::vector<QPointF> &tris, QPointF c, double r)
{
    for (int i = 0; i < kCircleSegments; ++i) {
        const double a0 = 2 * std::numbers::pi * i / kCircleSegments;
        const double a1 = 2 * std::numbers::pi * (i + 1) / kCircleSegments;
        tris.push_back(c);
        tris.emplace_back(c.x() + r * std::cos(a0), c.y() + r * std::sin(a0));
        tris.emplace_back(c.x() + r * std::cos(a1), c.y() + r * std::sin(a1));
    }
}

// Triangles for a stroke, for the live preview and pending strokes. Pen strokes
// get round joins and caps; highlighter strokes don't, because overlapping
// translucent triangles would show as darker spots.
inline std::vector<QPointF> strokeTriangles(const std::vector<core::StrokePoint> &points,
                                            float width, bool highlighter)
{
    std::vector<QPointF> tris;
    if (points.empty())
        return tris;
    const core::StrokeRibbon r = core::ribbon(points, width, !highlighter);
    if (points.size() == 1) {
        appendCircle(tris, { points[0].x, points[0].y }, r.radii[0]);
        return tris;
    }
    tris.reserve(points.size() * (highlighter ? 6 : 6 + 3 * kCircleSegments));
    for (std::size_t i = 0; i + 1 < points.size(); ++i) {
        tris.insert(tris.end(), { r.left[i], r.right[i], r.left[i + 1] });
        tris.insert(tris.end(), { r.right[i], r.right[i + 1], r.left[i + 1] });
    }
    if (!highlighter) {
        for (std::size_t i = 0; i < points.size(); ++i)
            appendCircle(tris, { points[i].x, points[i].y }, r.radii[i]);
    }
    return tris;
}

// Triangles in page pt, mapped to item px with `offset + pt * scale`.
// (Everything is placed in item coordinates rather than under a QSGTransformNode:
// the software scene graph renderer ignores the scale for image nodes.)
inline QSGGeometryNode *makeTriangleNode(const std::vector<QPointF> &tris, const QColor &color,
                                         QPointF offset, double scale)
{
    auto *geometry
        = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), static_cast<int>(tris.size()));
    geometry->setDrawingMode(QSGGeometry::DrawTriangles);
    QSGGeometry::Point2D *v = geometry->vertexDataAsPoint2D();
    for (std::size_t i = 0; i < tris.size(); ++i)
        v[i].set(static_cast<float>(offset.x() + tris[i].x() * scale),
                 static_cast<float>(offset.y() + tris[i].y() * scale));

    auto *material = new QSGFlatColorMaterial;
    material->setColor(color);

    auto *node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

// A ring (annulus) as a triangle strip, in item px.
inline QSGGeometryNode *makeRingNode(QPointF center, double inner, double outer,
                                     const QColor &color)
{
    constexpr int segments = 48;
    auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 2 * (segments + 1));
    geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);
    QSGGeometry::Point2D *v = geometry->vertexDataAsPoint2D();
    for (int i = 0; i <= segments; ++i) {
        const double a = 2 * std::numbers::pi * i / segments;
        const double c = std::cos(a), s = std::sin(a);
        v[2 * i].set(static_cast<float>(center.x() + inner * c),
                     static_cast<float>(center.y() + inner * s));
        v[2 * i + 1].set(static_cast<float>(center.x() + outer * c),
                         static_cast<float>(center.y() + outer * s));
    }
    auto *material = new QSGFlatColorMaterial;
    material->setColor(color);
    auto *node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

inline void deleteChildren(QSGNode *parent)
{
    while (QSGNode *child = parent->firstChild()) {
        parent->removeChildNode(child);
        delete child;
    }
}

// Scene graph layout of the canvas.
struct CanvasRoot : QSGNode
{
    QSGNode *frame = new QSGNode;
    QSGNode *page = new QSGNode;
    QSGNode *previousTiles = new QSGNode;
    QSGNode *tiles = new QSGNode;
    QSGNode *pending = new QSGNode;
    QSGNode *live = new QSGNode;
    QSGNode *selection = new QSGNode;
    QSGNode *cursor = new QSGNode;

    CanvasRoot()
    {
        appendChildNode(frame);
        appendChildNode(page);
        page->appendChildNode(previousTiles);
        page->appendChildNode(tiles);
        page->appendChildNode(pending);
        page->appendChildNode(live);
        appendChildNode(selection);
        appendChildNode(cursor);
    }
};

// A thick line segment as two triangles (pt or px, whatever the caller uses).
inline void appendSegment(std::vector<QPointF> &tris, QPointF a, QPointF b, double halfWidth)
{
    const QPointF d = b - a;
    const double len = std::hypot(d.x(), d.y());
    if (len <= 0.0)
        return;
    const QPointF n(-d.y() / len * halfWidth, d.x() / len * halfWidth);
    tris.insert(tris.end(), { a + n, a - n, b + n, a - n, b - n, b + n });
}

// Triangles for any object, in page pt (strokes only, for now).
inline std::vector<QPointF> objectTriangles(const core::Object &object)
{
    if (const auto *s = std::get_if<core::Stroke>(&object))
        return strokeTriangles(s->points, s->width, s->tool == core::StrokeTool::Highlighter);
    return { };
}

inline QColor objectColor(const core::Object &object)
{
    if (const auto *s = std::get_if<core::Stroke>(&object)) {
        QColor c(s->color.r, s->color.g, s->color.b);
        if (s->tool == core::StrokeTool::Highlighter)
            c.setAlphaF(0.45F);
        return c;
    }
    return Qt::black;
}

} // namespace canvasgeometry
