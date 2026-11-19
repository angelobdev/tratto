// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/document.h"

#include <cstddef>
#include <deque>
#include <memory>
#include <vector>

namespace tratto::core {

// A reversible change to a Document. apply() and revert() must be exact inverses,
// and both mark the pages they touch as dirty.
class Command
{
public:
    virtual ~Command() = default;
    virtual void apply(Document &doc) = 0;
    virtual void revert(Document &doc) = 0;
};

// Appends objects to the top of a page.
class AddObjects final : public Command
{
public:
    AddObjects(PageId page, std::vector<Object> objects);
    void apply(Document &doc) override;
    void revert(Document &doc) override;

private:
    PageId m_page;
    std::vector<Object> m_objects;
};

// Removes objects from a page; revert puts them back at their original positions.
class RemoveObjects final : public Command
{
public:
    RemoveObjects(PageId page, std::vector<ObjectId> ids);
    void apply(Document &doc) override;
    void revert(Document &doc) override;

private:
    PageId m_page;
    std::vector<ObjectId> m_ids;
    std::vector<std::pair<std::size_t, Object>> m_removed; // index, object; filled by apply
};

// Linear undo history for one document. push() applies a command; a new command
// discards anything that could have been redone.
class UndoStack
{
public:
    static constexpr std::size_t kDefaultLimit = 500;

    explicit UndoStack(std::size_t limit = kDefaultLimit);

    void push(Document &doc, std::unique_ptr<Command> command);
    bool undo(Document &doc);
    bool redo(Document &doc);

    [[nodiscard]] bool canUndo() const { return m_index > 0; }
    [[nodiscard]] bool canRedo() const { return m_index < m_commands.size(); }
    [[nodiscard]] std::size_t size() const { return m_commands.size(); }
    void clear();

private:
    std::deque<std::unique_ptr<Command>> m_commands;
    std::size_t m_index = 0; // commands before this index are applied
    std::size_t m_limit;
};

} // namespace tratto::core
