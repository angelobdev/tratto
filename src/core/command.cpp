// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/core/command.h"

#include <algorithm>

namespace tratto::core {

AddObjects::AddObjects(PageId page, std::vector<Object> objects)
    : m_page(page)
    , m_objects(std::move(objects))
{
}

void AddObjects::apply(Document &doc)
{
    Page *page = doc.page(m_page);
    if (!page)
        return;
    page->objects.insert(page->objects.end(), m_objects.begin(), m_objects.end());
    doc.dirtyPages.insert(m_page);
}

void AddObjects::revert(Document &doc)
{
    Page *page = doc.page(m_page);
    if (!page)
        return;
    std::erase_if(page->objects, [this](const Object &o) {
        return std::ranges::any_of(
            m_objects, [&](const Object &added) { return objectId(added) == objectId(o); });
    });
    doc.dirtyPages.insert(m_page);
}

RemoveObjects::RemoveObjects(PageId page, std::vector<ObjectId> ids)
    : m_page(page)
    , m_ids(std::move(ids))
{
}

void RemoveObjects::apply(Document &doc)
{
    Page *page = doc.page(m_page);
    if (!page)
        return;
    m_removed.clear();
    for (std::size_t i = page->objects.size(); i-- > 0;) {
        if (std::ranges::find(m_ids, objectId(page->objects[i])) != m_ids.end()) {
            m_removed.emplace_back(i, std::move(page->objects[i]));
            page->objects.erase(page->objects.begin() + static_cast<std::ptrdiff_t>(i));
        }
    }
    doc.dirtyPages.insert(m_page);
}

void RemoveObjects::revert(Document &doc)
{
    Page *page = doc.page(m_page);
    if (!page)
        return;
    // m_removed holds descending indices; reinsert in ascending order.
    for (auto it = m_removed.rbegin(); it != m_removed.rend(); ++it) {
        const auto index = static_cast<std::ptrdiff_t>(std::min(it->first, page->objects.size()));
        page->objects.insert(page->objects.begin() + index, it->second);
    }
    m_removed.clear();
    doc.dirtyPages.insert(m_page);
}

UndoStack::UndoStack(std::size_t limit)
    : m_limit(std::max<std::size_t>(limit, 1))
{
}

void UndoStack::push(Document &doc, std::unique_ptr<Command> command)
{
    command->apply(doc);
    m_commands.erase(m_commands.begin() + static_cast<std::ptrdiff_t>(m_index), m_commands.end());
    m_commands.push_back(std::move(command));
    if (m_commands.size() > m_limit)
        m_commands.pop_front();
    m_index = m_commands.size();
}

bool UndoStack::undo(Document &doc)
{
    if (!canUndo())
        return false;
    m_commands[--m_index]->revert(doc);
    return true;
}

bool UndoStack::redo(Document &doc)
{
    if (!canRedo())
        return false;
    m_commands[m_index++]->apply(doc);
    return true;
}

void UndoStack::clear()
{
    m_commands.clear();
    m_index = 0;
}

} // namespace tratto::core
