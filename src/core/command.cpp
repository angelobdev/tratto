// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/core/command.h"

#include <algorithm>

namespace tratto::core {

namespace {

QRectF boundsOf(const std::vector<Object> &objects)
{
    QRectF bounds;
    for (const Object &o : objects)
        bounds |= objectBounds(o);
    return bounds;
}

} // namespace

AddObjects::AddObjects(PageId page, std::vector<Object> objects)
    : m_page(page)
    , m_objects(std::move(objects))
    , m_bounds(boundsOf(m_objects))
{
}

Change AddObjects::apply(Document &doc)
{
    if (Page *page = doc.page(m_page)) {
        page->objects.insert(page->objects.end(), m_objects.begin(), m_objects.end());
        doc.dirtyPages.insert(m_page);
    }
    return { m_page, m_bounds };
}

Change AddObjects::revert(Document &doc)
{
    if (Page *page = doc.page(m_page)) {
        std::erase_if(page->objects, [this](const Object &o) {
            return std::ranges::any_of(
                m_objects, [&](const Object &added) { return objectId(added) == objectId(o); });
        });
        doc.dirtyPages.insert(m_page);
    }
    return { m_page, m_bounds };
}

RemoveObjects::RemoveObjects(PageId page, std::vector<ObjectId> ids)
    : m_page(page)
    , m_ids(std::move(ids))
{
}

Change RemoveObjects::apply(Document &doc)
{
    m_removed.clear();
    m_bounds = { };
    if (Page *page = doc.page(m_page)) {
        for (std::size_t i = page->objects.size(); i-- > 0;) {
            if (std::ranges::find(m_ids, objectId(page->objects[i])) != m_ids.end()) {
                m_bounds |= objectBounds(page->objects[i]);
                m_removed.emplace_back(i, std::move(page->objects[i]));
                page->objects.erase(page->objects.begin() + static_cast<std::ptrdiff_t>(i));
            }
        }
        doc.dirtyPages.insert(m_page);
    }
    return { m_page, m_bounds };
}

Change RemoveObjects::revert(Document &doc)
{
    if (Page *page = doc.page(m_page)) {
        // m_removed holds descending indices; reinsert in ascending order.
        for (auto it = m_removed.rbegin(); it != m_removed.rend(); ++it) {
            const auto index
                = static_cast<std::ptrdiff_t>(std::min(it->first, page->objects.size()));
            page->objects.insert(page->objects.begin() + index, it->second);
        }
        doc.dirtyPages.insert(m_page);
    }
    m_removed.clear();
    return { m_page, m_bounds };
}

AddPage::AddPage(Page page, std::size_t index)
    : m_page(std::move(page))
    , m_index(index)
{
}

Change AddPage::apply(Document &doc)
{
    const auto index = static_cast<std::ptrdiff_t>(std::min(m_index, doc.pages.size()));
    doc.pages.insert(doc.pages.begin() + index, m_page);
    doc.dirtyPages.insert(m_page.id);
    doc.infoDirty = true; // page order
    return { m_page.id, { } };
}

Change AddPage::revert(Document &doc)
{
    std::erase_if(doc.pages, [this](const Page &p) { return p.id == m_page.id; });
    doc.dirtyPages.erase(m_page.id);
    doc.infoDirty = true;
    return { m_page.id, { } };
}

Change CommandGroup::apply(Document &doc)
{
    Change total;
    for (const auto &command : m_commands) {
        const Change c = command->apply(doc);
        total.page = c.page;
        total.bounds |= c.bounds;
    }
    return total;
}

Change CommandGroup::revert(Document &doc)
{
    Change total;
    for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it) {
        const Change c = (*it)->revert(doc);
        total.page = c.page;
        total.bounds |= c.bounds;
    }
    return total;
}

UndoStack::UndoStack(std::size_t limit)
    : m_limit(std::max<std::size_t>(limit, 1))
{
}

Change UndoStack::push(Document &doc, std::unique_ptr<Command> command)
{
    const Change change = command->apply(doc);
    if (m_group)
        m_group->add(std::move(command));
    else
        record(std::move(command));
    return change;
}

void UndoStack::record(std::unique_ptr<Command> command)
{
    m_commands.erase(m_commands.begin() + static_cast<std::ptrdiff_t>(m_index), m_commands.end());
    m_commands.push_back(std::move(command));
    if (m_commands.size() > m_limit)
        m_commands.pop_front();
    m_index = m_commands.size();
}

void UndoStack::beginGroup()
{
    if (!m_group)
        m_group = std::make_unique<CommandGroup>();
}

void UndoStack::endGroup()
{
    if (!m_group)
        return;
    auto group = std::move(m_group);
    if (!group->empty())
        record(std::move(group)); // already applied
}

std::optional<Change> UndoStack::undo(Document &doc)
{
    if (!canUndo())
        return std::nullopt;
    return m_commands[--m_index]->revert(doc);
}

std::optional<Change> UndoStack::redo(Document &doc)
{
    if (!canRedo())
        return std::nullopt;
    return m_commands[m_index++]->apply(doc);
}

void UndoStack::clear()
{
    m_commands.clear();
    m_index = 0;
    m_group.reset();
}

} // namespace tratto::core
