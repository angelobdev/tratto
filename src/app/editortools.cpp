// SPDX-License-Identifier: GPL-3.0-or-later
#include "editortools.h"

#include "tratto/core/tools.h"

#include <algorithm>

namespace core = tratto::core;

namespace {

template<typename Array> QList<QColor> toColors(const Array &colors)
{
    QList<QColor> list;
    list.reserve(static_cast<qsizetype>(colors.size()));
    for (const core::Rgba c : colors)
        list.append(QColor(c.r, c.g, c.b));
    return list;
}

template<typename Array> float preset(const Array &values, int index)
{
    const auto last = static_cast<int>(values.size()) - 1;
    return values[static_cast<std::size_t>(std::clamp(index, 0, last))];
}

} // namespace

EditorTools::EditorTools(QObject *parent)
    : QObject(parent)
    , m_textSize(core::kDefaultTextSize)
{
}

void EditorTools::setTool(Tool tool)
{
    if (tool == m_tool)
        return;
    m_tool = tool;
    emit changed();
}

void EditorTools::setTextStyle(TextFont font, int size, bool bold, bool italic, int colorIndex)
{
    m_textFont = font;
    m_textSize = size;
    m_textBold = bold;
    m_textItalic = italic;
    if (colorIndex >= 0)
        m_textColor = colorIndex;
    emit changed();
}

QList<QColor> EditorTools::penColors() const
{
    return toColors(core::kPenColors);
}

QList<QColor> EditorTools::highlighterColors() const
{
    return toColors(core::kHighlighterColors);
}

QList<int> EditorTools::textSizes() const
{
    return { core::kTextSizes.begin(), core::kTextSizes.end() };
}

float EditorTools::penWidthPt() const
{
    return preset(core::kPenWidths, m_penWidth);
}

float EditorTools::highlighterWidthPt() const
{
    return preset(core::kHighlighterWidths, m_highlighterWidth);
}

float EditorTools::eraserRadiusPt() const
{
    return preset(core::kEraserRadii, m_eraserSize);
}
