// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/document.h"

#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <vector>

namespace tratto::core {

// What a command changed: one page and the area (pt) that needs repainting.
// An empty `bounds` means the whole page.
struct Change
{
    PageId page;
    QRectF bounds;
};

// A reversible change to a Document. apply() and revert() must be exact inverses,
// and both mark the pages they touch as dirty.
class Command
{
public:
    virtual ~Command() = default;
    virtual Change apply(Document &doc) = 0;
    virtual Change revert(Document &doc) = 0;
};

// Appends objects to the top of a page.
class AddObjects final : public Command
{
public:
    AddObjects(PageId page, std::vector<Object> objects);
    Change apply(Document &doc) override;
    Change revert(Document &doc) override;

private:
    PageId m_page;
    std::vector<Object> m_objects;
    QRectF m_bounds;
};

// Removes objects from a page; revert puts them back at their original positions.
class RemoveObjects final : public Command
{
public:
    RemoveObjects(PageId page, std::vector<ObjectId> ids);
    Change apply(Document &doc) override;
    Change revert(Document &doc) override;

private:
    PageId m_page;
    std::vector<ObjectId> m_ids;
    std::vector<std::pair<std::size_t, Object>> m_removed; // index, object; filled by apply
    QRectF m_bounds;
};

// Replaces objects with others in the same drawing position (partial eraser:
// a stroke becomes its remaining pieces). An empty replacement removes the object.
class ReplaceObjects final : public Command
{
public:
    struct Replacement
    {
        ObjectId original;
        std::vector<Object> pieces;
    };

    ReplaceObjects(PageId page, std::vector<Replacement> replacements);
    Change apply(Document &doc) override;
    Change revert(Document &doc) override;

private:
    PageId m_page;
    std::vector<Replacement> m_replacements;
    std::vector<std::pair<std::size_t, Object>> m_originals; // index, object; filled by apply
    QRectF m_bounds;
};

// Inserts a page at `index` (clamped to the end).
class AddPage final : public Command
{
public:
    AddPage(Page page, std::size_t index);
    Change apply(Document &doc) override;
    Change revert(Document &doc) override;

private:
    Page m_page;
    std::size_t m_index;
};

// Several commands undone and redone as one step (e.g. one eraser drag).
class CommandGroup final : public Command
{
public:
    void add(std::unique_ptr<Command> command) { m_commands.push_back(std::move(command)); }
    [[nodiscard]] bool empty() const { return m_commands.empty(); }
    Change apply(Document &doc) override;
    Change revert(Document &doc) override;

private:
    std::vector<std::unique_ptr<Command>> m_commands;
};

// Linear undo history for one document. push() applies a command; a new command
// discards anything that could have been redone.
class UndoStack
{
public:
    static constexpr std::size_t kDefaultLimit = 500;

    explicit UndoStack(std::size_t limit = kDefaultLimit);

    Change push(Document &doc, std::unique_ptr<Command> command);
    std::optional<Change> undo(Document &doc);
    std::optional<Change> redo(Document &doc);

    // Between beginGroup() and endGroup(), pushed commands are applied at once
    // but recorded as a single undo step. Empty groups leave no step.
    void beginGroup();
    void endGroup();
    [[nodiscard]] bool inGroup() const { return m_group != nullptr; }

    [[nodiscard]] bool canUndo() const { return !inGroup() && m_index > 0; }
    [[nodiscard]] bool canRedo() const { return !inGroup() && m_index < m_commands.size(); }
    [[nodiscard]] std::size_t size() const { return m_commands.size(); }
    void clear();

private:
    void record(std::unique_ptr<Command> command);

    std::deque<std::unique_ptr<Command>> m_commands;
    std::size_t m_index = 0; // commands before this index are applied
    std::size_t m_limit;
    std::unique_ptr<CommandGroup> m_group;
};

} // namespace tratto::core
