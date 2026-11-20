// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/storage/note_store.h"

#include <QBuffer>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSaveFile>

#include <algorithm>

Q_LOGGING_CATEGORY(lcStorage, "tratto.storage")

namespace tratto::storage {

namespace {

Error ioError(const QString &detail)
{
    return { ErrorCode::Io, detail };
}

// Atomic write: temporary file in the same folder, fsync, rename.
Result<void> writeAtomically(const QString &path, const QByteArray &data)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        return std::unexpected(ioError(QStringLiteral("mkpath ") + path));
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return std::unexpected(ioError(file.errorString() + QStringLiteral(": ") + path));
    if (file.write(data) != data.size()) {
        file.cancelWriting();
        return std::unexpected(ioError(file.errorString() + QStringLiteral(": ") + path));
    }
    if (!file.commit())
        return std::unexpected(ioError(file.errorString() + QStringLiteral(": ") + path));
    return { };
}

Result<QByteArray> readFile(const QString &path)
{
    QFile file(path);
    if (!file.exists())
        return std::unexpected(Error { ErrorCode::NotFound, path });
    if (!file.open(QIODevice::ReadOnly))
        return std::unexpected(ioError(file.errorString() + QStringLiteral(": ") + path));
    return file.readAll();
}

} // namespace

SaveRequest takeSaveRequest(core::Document &doc, bool writeAllPages)
{
    SaveRequest request;
    request.info = doc.info;
    request.info.pageCount = static_cast<int>(doc.pages.size());
    for (const core::Page &page : doc.pages) {
        request.pageOrder.push_back(page.id);
        if (writeAllPages || doc.dirtyPages.contains(page.id))
            request.pages.push_back(page);
    }
    doc.dirtyPages.clear();
    doc.infoDirty = false;
    return request;
}

NoteStore::NoteStore(QString root)
    : m_root(std::move(root))
{
}

QString NoteStore::noteDir(core::NoteId id) const
{
    return m_root + QStringLiteral("/notes/") + idToString(id.value);
}

QString NoteStore::manifestPath(core::NoteId id) const
{
    return noteDir(id) + QStringLiteral("/manifest.json");
}

QString NoteStore::pagePath(core::NoteId note, core::PageId page) const
{
    return noteDir(note) + QStringLiteral("/pages/") + idToString(page.value)
        + QStringLiteral(".cbor");
}

QString NoteStore::thumbnailPath(core::NoteId id) const
{
    return noteDir(id) + QStringLiteral("/thumb.png");
}

Result<Manifest> NoteStore::readManifest(const QString &path) const
{
    const auto bytes = readFile(path);
    if (!bytes)
        return std::unexpected(bytes.error());
    return decodeManifest(*bytes);
}

Result<void> NoteStore::save(const SaveRequest &request)
{
    const core::NoteId id = request.info.id;
    for (const core::Page &page : request.pages) {
        if (auto written = writeAtomically(pagePath(id, page.id), encodePage(page)); !written)
            return written;
    }

    Manifest manifest { request.info, request.pageOrder };
    manifest.info.pageCount = static_cast<int>(request.pageOrder.size());
    if (auto written = writeAtomically(manifestPath(id), encodeManifest(manifest)); !written)
        return written;

    // Page files no longer referenced (deleted pages) go only after the manifest.
    QDir pages(noteDir(id) + QStringLiteral("/pages"));
    for (const QString &name : pages.entryList({ QStringLiteral("*.cbor") }, QDir::Files)) {
        const core::PageId pageId { QUuid::fromString(name.chopped(5)) };
        if (std::ranges::find(request.pageOrder, pageId) == request.pageOrder.end())
            pages.remove(name);
    }
    return { };
}

Result<void> NoteStore::updateInfo(const core::NoteInfo &info)
{
    auto manifest = readManifest(manifestPath(info.id));
    if (!manifest)
        return std::unexpected(manifest.error());
    manifest->info = info;
    manifest->info.pageCount = static_cast<int>(manifest->pages.size());
    return writeAtomically(manifestPath(info.id), encodeManifest(*manifest));
}

Result<core::Document> NoteStore::load(core::NoteId id) const
{
    auto manifest = readManifest(manifestPath(id));
    if (!manifest)
        return std::unexpected(manifest.error());

    core::Document doc;
    doc.info = manifest->info;
    for (const core::PageId &pageId : manifest->pages) {
        const auto bytes = readFile(pagePath(id, pageId));
        if (!bytes)
            return std::unexpected(bytes.error());
        auto page = decodePage(*bytes);
        if (!page)
            return std::unexpected(page.error());
        doc.pages.push_back(std::move(*page));
    }
    return doc;
}

std::vector<core::NoteInfo> NoteStore::list() const
{
    std::vector<core::NoteInfo> notes;
    QDir dir(m_root + QStringLiteral("/notes"));
    for (const QString &name : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const auto manifest = readManifest(dir.filePath(name) + QStringLiteral("/manifest.json"));
        if (manifest)
            notes.push_back(manifest->info);
        else
            qCWarning(lcStorage) << "Skipping note" << name << manifest.error().detail;
    }
    return notes;
}

Result<void> NoteStore::remove(core::NoteId id)
{
    QDir dir(noteDir(id));
    if (!dir.exists())
        return std::unexpected(Error { ErrorCode::NotFound, dir.path() });
    if (!dir.removeRecursively())
        return std::unexpected(ioError(QStringLiteral("remove ") + dir.path()));
    return { };
}

Result<void> NoteStore::saveThumbnail(core::NoteId id, const QImage &image)
{
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG"))
        return std::unexpected(ioError(QStringLiteral("encode thumbnail")));
    return writeAtomically(thumbnailPath(id), png);
}

Result<std::vector<core::Group>> NoteStore::loadGroups() const
{
    const auto bytes = readFile(m_root + QStringLiteral("/groups.json"));
    if (!bytes) {
        if (bytes.error().code == ErrorCode::NotFound)
            return std::vector<core::Group> { };
        return std::unexpected(bytes.error());
    }
    return decodeGroups(*bytes);
}

Result<void> NoteStore::saveGroups(const std::vector<core::Group> &groups)
{
    return writeAtomically(m_root + QStringLiteral("/groups.json"), encodeGroups(groups));
}

int NoteStore::removeTemporaryFiles()
{
    // QSaveFile writes to "<name>.XXXXXX" next to the target and renames on commit.
    static const QRegularExpression temporary(QStringLiteral(R"(\.(json|cbor|png)\.\w{6}$)"));
    int removed = 0;
    QDirIterator it(m_root, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        if (temporary.match(path).hasMatch() && QFile::remove(path))
            ++removed;
    }
    return removed;
}

int NoteStore::purgeTrash(const QDateTime &now, int days)
{
    int purged = 0;
    for (const core::NoteInfo &info : list()) {
        if (info.deleted && info.deleted->daysTo(now) >= days && remove(info.id))
            ++purged;
    }
    return purged;
}

} // namespace tratto::storage
