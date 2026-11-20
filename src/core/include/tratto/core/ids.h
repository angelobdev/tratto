// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QUuid>

namespace tratto::core {

// A UUIDv7: random, but ordered by creation time (RFC 9562).
// QUuid::createUuidV7() needs Qt 6.9; the minimum is 6.8.
[[nodiscard]] QUuid newId();

// Distinct id types, so a page id can't be passed where a stroke id is expected.
template<typename Tag> struct Id
{
    QUuid value;

    static Id create() { return { newId() }; }
    bool isNull() const { return value.isNull(); }

    friend bool operator==(const Id &, const Id &) = default;
    friend auto operator<=>(const Id &a, const Id &b) { return a.value <=> b.value; }
};

using ObjectId = Id<struct ObjectTag>;
using PageId = Id<struct PageTag>;
using NoteId = Id<struct NoteTag>;
using GroupId = Id<struct GroupTag>;

} // namespace tratto::core
