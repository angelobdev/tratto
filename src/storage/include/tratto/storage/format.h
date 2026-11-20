// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/document.h"

#include <QByteArray>
#include <QJsonObject>
#include <QString>

#include <expected>
#include <vector>

// On-disk format of note bundles (ARCHITECTURE §8).
//
//   manifest.json   NoteInfo + page order, with "format_version"
//   pages/<id>.cbor one page: size, paper, objects
//
// Every change to this format needs: kFormatVersion + 1, a step in migrateManifest(),
// and a round-trip test (tests/storage).
namespace tratto::storage {

// 1: strokes. 2: text boxes.
inline constexpr int kFormatVersion = 2;

enum class ErrorCode { Io, Corrupt, NewerVersion, NotFound };

struct Error
{
    ErrorCode code;
    QString detail; // for logs, not for the UI

    friend bool operator==(const Error &, const Error &) = default;
};

template<typename T> using Result = std::expected<T, Error>;

struct Manifest
{
    core::NoteInfo info;
    std::vector<core::PageId> pages; // order
};

[[nodiscard]] QByteArray encodeManifest(const Manifest &manifest);
[[nodiscard]] Result<Manifest> decodeManifest(const QByteArray &json);

// Brings an older manifest up to kFormatVersion. Fails for newer versions.
[[nodiscard]] Result<QJsonObject> migrateManifest(QJsonObject manifest);

[[nodiscard]] QByteArray encodePage(const core::Page &page);
[[nodiscard]] Result<core::Page> decodePage(const QByteArray &cbor);

[[nodiscard]] QByteArray encodeGroups(const std::vector<core::Group> &groups);
[[nodiscard]] Result<std::vector<core::Group>> decodeGroups(const QByteArray &json);

[[nodiscard]] QString idToString(const QUuid &id);

} // namespace tratto::storage
