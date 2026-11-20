// SPDX-License-Identifier: GPL-3.0-or-later
// CanvasItem: the text tool. A tap creates a text box or opens an existing one;
// the QML editor (TextBoxEditor.qml) does the typing and calls commitText().
#include "canvasitem.h"
#include "tratto/core/command.h"
#include "tratto/render/text_painter.h"

#include <algorithm>

namespace core = tratto::core;

namespace {

constexpr double kDefaultWidthPt = 400.0; // new boxes; narrower near the right edge
constexpr double kPageMarginPt = 24.0;
constexpr double kMinWidthPt = 80.0;

QColor toQColor(core::Rgba c)
{
    return QColor(c.r, c.g, c.b);
}

core::Rgba toRgba(const QColor &color)
{
    const QColor c = color.toRgb();
    return { static_cast<std::uint8_t>(c.red()), static_cast<std::uint8_t>(c.green()),
             static_cast<std::uint8_t>(c.blue()), 255 };
}

} // namespace

QRectF CanvasItem::textEditRect() const
{
    if (!m_editing)
        return { };
    const QRectF pt = m_editing->bbox;
    return QRectF(m_offset + pt.topLeft() * scale(), pt.size() * scale());
}

void CanvasItem::beginText(QPointF itemPos)
{
    if (m_editing) {
        // A tap elsewhere ends the current edit; the next tap starts a new one.
        emit textCommitRequested(); // the editor commits what it has (synchronously)
        return;
    }
    const core::Page *page = m_session ? m_session->page(m_pageIndex) : nullptr;
    if (!page || !m_tools)
        return;
    const QPointF pt = toPage(itemPos);

    // An existing box under the tap: edit it (topmost first).
    for (auto it = page->objects.rbegin(); it != page->objects.rend(); ++it) {
        const auto *text = std::get_if<core::TextBox>(&*it);
        if (text && text->bbox.contains(pt)) {
            m_editing = *text;
            m_editingIsNew = false;
            // The toolbar shows (and then edits) this box's style.
            const auto colors = m_tools->penColors();
            const auto match = std::ranges::find(colors, toQColor(text->color));
            m_tools->setTextStyle(
                static_cast<EditorTools::TextFont>(text->font),
                static_cast<int>(std::lround(text->size)), text->bold, text->italic,
                match != colors.end() ? static_cast<int>(match - colors.begin()) : -1);
            ++m_generation;
            refreshSnapshot(); // hide the original while it is edited
            invalidate(text->bbox);
            emit textEditChanged();
            emit textEditStarted();
            return;
        }
    }

    // A new box: the first line centered on the tap.
    core::TextBox box;
    box.id = core::ObjectId::create();
    box.size = static_cast<float>(m_tools->textSize());
    const double lineHeight = tratto::render::textHeight(box);
    const double width = std::max(
        kMinWidthPt, std::min(kDefaultWidthPt, page->size.width() - pt.x() - kPageMarginPt));
    box.bbox = QRectF(pt.x(), pt.y() - lineHeight / 2, width, lineHeight);
    m_editing = box;
    m_editingIsNew = true;
    emit textEditChanged();
    emit textEditStarted();
}

void CanvasItem::commitText(const QString &text)
{
    if (!m_editing || !m_session || !m_tools)
        return;
    core::TextBox box = *m_editing;
    const bool isNew = m_editingIsNew;
    endTextEditing();

    const core::Page *page = m_session->page(m_pageIndex);
    if (!page)
        return;
    // The style is the toolbar's: it was set from the box when editing started.
    box.text = text;
    box.font = static_cast<core::TextFont>(m_tools->textFont());
    box.size = static_cast<float>(m_tools->textSize());
    box.bold = m_tools->textBold();
    box.italic = m_tools->textItalic();
    box.color = toRgba(m_tools->penColors().value(m_tools->textColorIndex(), Qt::black));
    box.bbox.setHeight(tratto::render::textHeight(box));

    const bool empty = text.trimmed().isEmpty();
    if (isNew) {
        if (!empty)
            m_session->push(
                std::make_unique<core::AddObjects>(page->id, std::vector<core::Object> { box }));
        return;
    }
    const core::Object *original = page->find(box.id);
    if (!original)
        return;
    if (empty) {
        m_session->push(std::make_unique<core::RemoveObjects>(page->id, std::vector { box.id }));
        return;
    }
    const auto &before = std::get<core::TextBox>(*original);
    const bool changed = before.text != box.text || before.font != box.font
        || before.size != box.size || before.bold != box.bold || before.italic != box.italic
        || before.color != box.color;
    if (changed) {
        std::vector<core::ReplaceObjects::Replacement> edit;
        edit.push_back({ box.id, { box } });
        m_session->push(std::make_unique<core::ReplaceObjects>(page->id, std::move(edit)));
    }
}

void CanvasItem::endTextEditing()
{
    if (!m_editing)
        return;
    const QRectF area = m_editing->bbox;
    m_editing.reset();
    ++m_generation;
    refreshSnapshot(); // the original (if any) comes back
    invalidate(area);
    update();
    emit textEditChanged();
}
