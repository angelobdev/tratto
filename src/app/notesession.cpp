// SPDX-License-Identifier: GPL-3.0-or-later
#include "notesession.h"

#include <algorithm>

namespace core = tratto::core;

NoteSession::NoteSession(QObject *parent)
    : QObject(parent)
{
}

void NoteSession::createBlank(int pattern, qreal spacingMm, const QColor &paperColor, int pages)
{
    core::PaperStyle paper;
    paper.background = { static_cast<core::PatternKind>(std::clamp(pattern, 0, 3)),
                         static_cast<float>(spacingMm) };
    const QColor rgb = paperColor.toRgb();
    paper.paperColor
        = { static_cast<std::uint8_t>(rgb.red()), static_cast<std::uint8_t>(rgb.green()),
            static_cast<std::uint8_t>(rgb.blue()), 255 };

    m_document = { };
    m_document.info.id = core::NoteId::create();
    for (int i = 0; i < std::max(pages, 1); ++i)
        m_document.pages.push_back(core::Page::create(paper));
    m_history.clear();

    emit pagesChanged();
    emit historyChanged();
    for (int i = 0; i < pageCount(); ++i)
        emit pageChanged(i, { });
}

const core::Page *NoteSession::page(int index) const
{
    if (index < 0 || index >= pageCount())
        return nullptr;
    return &m_document.pages[static_cast<std::size_t>(index)];
}

int NoteSession::pageIndex(core::PageId id) const
{
    const auto it
        = std::ranges::find_if(m_document.pages, [id](const core::Page &p) { return p.id == id; });
    return it == m_document.pages.end() ? -1 : static_cast<int>(it - m_document.pages.begin());
}

void NoteSession::push(std::unique_ptr<core::Command> command)
{
    notify(m_history.push(m_document, std::move(command)));
    if (!m_history.inGroup())
        emit historyChanged();
}

void NoteSession::beginGroup()
{
    m_history.beginGroup();
    emit historyChanged();
}

void NoteSession::endGroup()
{
    m_history.endGroup();
    emit historyChanged();
}

void NoteSession::undo()
{
    if (const auto change = m_history.undo(m_document)) {
        notify(*change);
        emit historyChanged();
    }
}

void NoteSession::redo()
{
    if (const auto change = m_history.redo(m_document)) {
        notify(*change);
        emit historyChanged();
    }
}

void NoteSession::notify(const core::Change &change)
{
    const int index = pageIndex(change.page);
    if (index >= 0)
        emit pageChanged(index, change.bounds);
}
