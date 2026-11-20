// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/render/text_painter.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QTextLayout>

namespace tratto::render {

namespace {

// Text is laid out at this pixel size and scaled to the box's size in pt, so it
// looks the same at any zoom and in export (QFont pixel sizes are integers).
constexpr int kLayoutPixelSize = 100;

QFont layoutFont(const core::TextBox &box)
{
    QFont font;
    font.setFamilies(fontFamilies(box.font));
    switch (box.font) {
    case core::TextFont::Sans: font.setStyleHint(QFont::SansSerif); break;
    case core::TextFont::Serif: font.setStyleHint(QFont::Serif); break;
    case core::TextFont::Monospace:
        font.setStyleHint(QFont::TypeWriter);
        font.setFixedPitch(true);
        break;
    }
    font.setPixelSize(kLayoutPixelSize);
    font.setBold(box.bold);
    font.setItalic(box.italic);
    font.setHintingPreference(QFont::PreferNoHinting); // scales linearly
    return font;
}

double scaleOf(const core::TextBox &box)
{
    return static_cast<double>(box.size) / kLayoutPixelSize;
}

// Lays out the text in layout units; returns the total height.
double layout(QTextLayout &textLayout, const core::TextBox &box)
{
    QString text = box.text;
    text.replace(QLatin1Char('\n'), QChar::LineSeparator);
    textLayout.setText(text);
    textLayout.setFont(layoutFont(box));
    QTextOption option;
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textLayout.setTextOption(option);

    const double width = box.bbox.width() / scaleOf(box);
    const double lineHeight = QFontMetricsF(layoutFont(box)).lineSpacing();
    double y = 0.0;
    textLayout.beginLayout();
    for (QTextLine line = textLayout.createLine(); line.isValid(); line = textLayout.createLine()) {
        line.setLineWidth(width);
        line.setPosition(QPointF(0.0, y));
        y += lineHeight;
    }
    textLayout.endLayout();
    return std::max(y, lineHeight);
}

} // namespace

QStringList fontFamilies(core::TextFont font)
{
    switch (font) {
    case core::TextFont::Serif: return { QStringLiteral("serif") };
    case core::TextFont::Monospace: return { QStringLiteral("monospace") };
    case core::TextFont::Sans: break;
    }
    return { QStringLiteral("Inter Variable"), QStringLiteral("Inter"),
             QStringLiteral("sans-serif") };
}

double textHeight(const core::TextBox &box)
{
    QTextLayout textLayout;
    return layout(textLayout, box) * scaleOf(box);
}

void paintTextBox(QPainter &painter, const core::TextBox &box)
{
    QTextLayout textLayout;
    layout(textLayout, box);
    painter.save();
    painter.translate(box.bbox.topLeft());
    painter.scale(scaleOf(box), scaleOf(box));
    painter.setPen(QColor(box.color.r, box.color.g, box.color.b));
    textLayout.draw(&painter, QPointF(0, 0));
    painter.restore();
}

} // namespace tratto::render
