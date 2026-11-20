// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/storage/format.h"

#include <QDateTime>
#include <QImage>
#include <QString>

#include <vector>

namespace tratto::storage {

// What to write for one save: taken from a Document on the GUI thread, written on
// the I/O thread. Only the listed pages are rewritten.
struct SaveRequest
{
    core::NoteInfo info;
    std::vector<core::PageId> pageOrder;
    std::vector<core::Page> pages;
};

// Copies the unsaved parts of `doc` into a request and clears its dirty flags.
// `writeAllPages` is for a note that has never been saved.
[[nodiscard]] SaveRequest takeSaveRequest(core::Document &doc, bool writeAllPages = false);

// Notes on disk, one bundle per note:
//
//   <root>/groups.json
//   <root>/notes/<note-id>/manifest.json
//   <root>/notes/<note-id>/pages/<page-id>.cbor
//   <root>/notes/<note-id>/thumb.png
//
// Bundles are the source of truth. Every write is atomic (QSaveFile: temporary
// file, fsync, rename), and imported originals are never modified.
// Not thread-safe: use one instance from one thread (the I/O thread).
class NoteStore
{
public:
    explicit NoteStore(QString root);

    [[nodiscard]] const QString &root() const { return m_root; }

    // Writes the request's pages, then the manifest, then removes page files that
    // the manifest no longer lists. A crash at any point leaves a loadable note.
    Result<void> save(const SaveRequest &request);
    // Rewrites only the manifest (rename, move to a group, trash), keeping page order.
    Result<void> updateInfo(const core::NoteInfo &info);

    Result<core::Document> load(core::NoteId id) const;
    // Metadata of every readable note, trashed ones included. Unreadable bundles are
    // skipped (and logged).
    std::vector<core::NoteInfo> list() const;
    // Deletes a note's bundle for good.
    Result<void> remove(core::NoteId id);

    Result<void> saveThumbnail(core::NoteId id, const QImage &image);
    [[nodiscard]] QString thumbnailPath(core::NoteId id) const;

    Result<std::vector<core::Group>> loadGroups() const;
    Result<void> saveGroups(const std::vector<core::Group> &groups);

    // Removes temporary files left by an interrupted write. Returns how many.
    int removeTemporaryFiles();
    // Deletes notes trashed more than `days` days before `now`. Returns how many.
    int purgeTrash(const QDateTime &now, int days = 30);

private:
    [[nodiscard]] QString noteDir(core::NoteId id) const;
    [[nodiscard]] QString manifestPath(core::NoteId id) const;
    [[nodiscard]] QString pagePath(core::NoteId note, core::PageId page) const;
    Result<Manifest> readManifest(const QString &path) const;

    QString m_root;
};

} // namespace tratto::storage
