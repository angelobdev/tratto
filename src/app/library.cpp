// SPDX-License-Identifier: GPL-3.0-or-later
#include "library.h"

#include "tratto/core/color.h"
#include "tratto/render/page_painter.h"

#include <QJSEngine>
#include <QLoggingCategory>
#include <QPainter>
#include <QPointer>
#include <QUrl>

#include <algorithm>

Q_LOGGING_CATEGORY(lcLibrary, "tratto.library")

namespace core = tratto::core;
namespace storage = tratto::storage;

namespace {

Library *s_instance = nullptr;

// Library thumbnails: the first page, this many pixels wide.
constexpr int kThumbnailWidth = 480;

QString toString(const QUuid &id)
{
    return storage::idToString(id);
}

QUuid toUuid(const QString &id)
{
    return QUuid::fromString(id);
}

core::Rgba toRgba(const QColor &color)
{
    const QColor c = color.toRgb();
    return { static_cast<std::uint8_t>(c.red()), static_cast<std::uint8_t>(c.green()),
             static_cast<std::uint8_t>(c.blue()), 255 };
}

QColor toQColor(core::Rgba c)
{
    return QColor(c.r, c.g, c.b);
}

QImage renderThumbnail(const core::Page &page)
{
    const double scale = kThumbnailWidth / page.size.width();
    QImage image(QSize(kThumbnailWidth, static_cast<int>(page.size.height() * scale)),
                 QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(scale, scale);
    tratto::render::paintPage(painter, page);
    return image;
}

} // namespace

Library::Library(QString dataDir, QObject *parent)
    : QObject(parent)
    , m_store(std::make_shared<storage::NoteStore>(std::move(dataDir)))
{
    m_ioThread.setObjectName(QStringLiteral("tratto-io"));
    m_io = new QObject; // deleted when the thread finishes
    m_io->moveToThread(&m_ioThread);
    connect(&m_ioThread, &QThread::finished, m_io, &QObject::deleteLater);
    m_ioThread.start();

    // Startup: clean up after a crash, purge old trash, read everything.
    onIoThread([this, store = m_store] {
        const int leftovers = store->removeTemporaryFiles();
        const int purged = store->purgeTrash(QDateTime::currentDateTimeUtc());
        auto notes = store->list();
        auto groups = store->loadGroups();
        if (leftovers || purged)
            qCInfo(lcLibrary) << "Removed" << leftovers << "temporary files, purged" << purged
                              << "notes";
        QMetaObject::invokeMethod(
            this,
            [this, notes = std::move(notes), groups = std::move(groups)]() mutable {
                for (const core::NoteInfo &info : notes)
                    m_entries.insert(toString(info.id.value), { info, 1 });
                if (groups)
                    m_groups = std::move(*groups);
                else
                    qCWarning(lcLibrary) << "Could not read groups:" << groups.error().detail;
                std::ranges::sort(m_groups, { }, &core::Group::order);
                m_ready = true;
                refreshNotesCache();
                emit groupsChanged();
                emit readyChanged();
            },
            Qt::QueuedConnection);
    });
}

Library::~Library()
{
    // Let every queued save finish, then stop the thread.
    QMetaObject::invokeMethod(m_io, [] { QThread::currentThread()->quit(); }, Qt::QueuedConnection);
    m_ioThread.wait();
}

Library *Library::create(QQmlEngine *, QJSEngine *)
{
    Q_ASSERT(s_instance); // set in main() before QML loads
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

void Library::setInstance(Library *instance)
{
    s_instance = instance;
}

Library *Library::instance()
{
    return s_instance;
}

template<typename F> void Library::onIoThread(F &&work)
{
    QMetaObject::invokeMethod(m_io, std::forward<F>(work), Qt::QueuedConnection);
}

// ---------------------------------------------------------------------------
// Notes

QVariantMap Library::toVariant(const Entry &entry) const
{
    const core::NoteInfo &info = entry.info;
    const QString thumb = m_store->thumbnailPath(info.id);
    return {
        { QStringLiteral("id"), toString(info.id.value) },
        { QStringLiteral("title"), info.title },
        { QStringLiteral("group"), info.group ? toString(info.group->value) : QString() },
        { QStringLiteral("created"), info.created.toLocalTime() },
        { QStringLiteral("modified"), info.modified.toLocalTime() },
        { QStringLiteral("inTrash"), info.inTrash() },
        { QStringLiteral("deleted"), info.deleted ? info.deleted->toLocalTime() : QDateTime() },
        { QStringLiteral("pageCount"), info.pageCount },
        { QStringLiteral("paper"),
          QVariantMap {
              { QStringLiteral("pattern"), static_cast<int>(info.defaultPaper.background.kind) },
              { QStringLiteral("spacingMm"), info.defaultPaper.background.spacingMm },
              { QStringLiteral("color"), toQColor(info.defaultPaper.paperColor) },
          } },
        // The revision makes QML reload the image after each save.
        { QStringLiteral("thumbnail"),
          QUrl::fromLocalFile(thumb).toString() + QStringLiteral("?v=")
              + QString::number(entry.thumbnailRevision) },
        { QStringLiteral("source"), QString() }, // imported PDF / image, from M4
    };
}

void Library::refreshNotesCache()
{
    std::vector<const Entry *> sorted;
    for (const Entry &e : std::as_const(m_entries))
        sorted.push_back(&e);
    std::ranges::sort(
        sorted, [](const Entry *a, const Entry *b) { return a->info.modified > b->info.modified; });
    m_notesCache.clear();
    for (const Entry *e : sorted)
        m_notesCache.append(toVariant(*e));
    emit notesChanged();
}

QString Library::createNote(const QString &title, const QString &groupId, int pattern,
                            qreal spacingMm, const QColor &paperColor, int format, int orientation)
{
    core::PaperStyle paper;
    paper.background = { static_cast<core::PatternKind>(std::clamp(pattern, 0, 3)),
                         static_cast<float>(spacingMm) };
    paper.paperColor = toRgba(paperColor);
    paper.format = static_cast<core::PageFormat>(std::clamp(format, 0, 2));
    paper.orientation = static_cast<core::Orientation>(std::clamp(orientation, 0, 1));

    core::Document doc;
    doc.info.id = core::NoteId::create();
    doc.info.title = title;
    if (!groupId.isEmpty())
        doc.info.group = core::GroupId { toUuid(groupId) };
    doc.info.created = doc.info.modified = QDateTime::currentDateTimeUtc();
    doc.info.defaultPaper = paper;
    doc.pages.push_back(core::Page::create(paper));
    doc.info.pageCount = 1;

    const QString id = toString(doc.info.id.value);
    m_entries.insert(id, { doc.info, 0 });
    refreshNotesCache();
    save(storage::takeSaveRequest(doc, true), nullptr, { });
    return id;
}

void Library::updateInfo(const QString &id, const std::function<void(core::NoteInfo &)> &change)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end())
        return;
    change(it->info);
    refreshNotesCache();
    onIoThread([this, store = m_store, info = it->info] {
        if (auto saved = store->updateInfo(info); !saved) {
            const QString detail = saved.error().detail;
            QMetaObject::invokeMethod(
                this, [this, detail] { emit storageError(detail); }, Qt::QueuedConnection);
        }
    });
}

void Library::renameNote(const QString &id, const QString &title)
{
    updateInfo(id, [&](core::NoteInfo &info) { info.title = title.trimmed(); });
}

void Library::moveNote(const QString &id, const QString &groupId)
{
    updateInfo(id, [&](core::NoteInfo &info) {
        if (groupId.isEmpty())
            info.group.reset();
        else
            info.group = core::GroupId { toUuid(groupId) };
    });
}

void Library::trashNote(const QString &id)
{
    updateInfo(id, [](core::NoteInfo &info) { info.deleted = QDateTime::currentDateTimeUtc(); });
}

void Library::restoreNote(const QString &id)
{
    updateInfo(id, [](core::NoteInfo &info) { info.deleted.reset(); });
}

void Library::deleteNoteForever(const QString &id)
{
    const auto it = m_entries.find(id);
    if (it == m_entries.end())
        return;
    const core::NoteId noteId = it->info.id;
    m_entries.erase(it);
    refreshNotesCache();
    onIoThread([store = m_store, noteId] { (void)store->remove(noteId); });
}

void Library::emptyTrash()
{
    QStringList trashed;
    for (auto it = m_entries.cbegin(); it != m_entries.cend(); ++it) {
        if (it->info.inTrash())
            trashed.append(it.key());
    }
    for (const QString &id : trashed)
        deleteNoteForever(id);
}

void Library::noteInfoChanged(const core::NoteInfo &info)
{
    Entry &entry = m_entries[toString(info.id.value)];
    entry.info = info;
    ++entry.thumbnailRevision;
    refreshNotesCache();
}

// ---------------------------------------------------------------------------
// Groups

QVariantList Library::groups() const
{
    QVariantList list;
    for (const core::Group &g : m_groups) {
        list.append(QVariantMap { { QStringLiteral("id"), toString(g.id.value) },
                                  { QStringLiteral("name"), g.name },
                                  { QStringLiteral("color"), toQColor(g.color) } });
    }
    return list;
}

QVariantMap Library::group(const QString &id) const
{
    const QUuid uuid = toUuid(id);
    for (const core::Group &g : m_groups) {
        if (g.id.value == uuid)
            return { { QStringLiteral("id"), id },
                     { QStringLiteral("name"), g.name },
                     { QStringLiteral("color"), toQColor(g.color) } };
    }
    return { };
}

QList<QColor> Library::groupColors() const
{
    QList<QColor> colors;
    for (const core::Rgba c : core::kGroupColors)
        colors.append(toQColor(c));
    return colors;
}

void Library::saveGroups()
{
    emit groupsChanged();
    onIoThread([this, store = m_store, groups = m_groups] {
        if (auto saved = store->saveGroups(groups); !saved) {
            const QString detail = saved.error().detail;
            QMetaObject::invokeMethod(
                this, [this, detail] { emit storageError(detail); }, Qt::QueuedConnection);
        }
    });
}

QString Library::createGroup(const QString &name, const QColor &color)
{
    core::Group g;
    g.id = core::GroupId::create();
    g.name = name.trimmed();
    g.color = toRgba(color);
    g.order = m_groups.empty() ? 0 : m_groups.back().order + 1;
    m_groups.push_back(g);
    saveGroups();
    return toString(g.id.value);
}

void Library::renameGroup(const QString &id, const QString &name)
{
    const QUuid uuid = toUuid(id);
    for (core::Group &g : m_groups) {
        if (g.id.value == uuid)
            g.name = name.trimmed();
    }
    saveGroups();
}

void Library::deleteGroup(const QString &id)
{
    const QUuid uuid = toUuid(id);
    std::erase_if(m_groups, [&](const core::Group &g) { return g.id.value == uuid; });
    saveGroups();
    QStringList members;
    for (auto it = m_entries.cbegin(); it != m_entries.cend(); ++it) {
        if (it->info.group && it->info.group->value == uuid)
            members.append(it.key());
    }
    for (const QString &note : members)
        moveNote(note, { });
}

// ---------------------------------------------------------------------------
// Editor

void Library::load(core::NoteId id, QObject *context, LoadCallback callback)
{
    QPointer<QObject> guard(context);
    onIoThread([this, store = m_store, id, guard, callback = std::move(callback)]() mutable {
        auto result = store->load(id);
        QMetaObject::invokeMethod(
            this,
            [guard, callback = std::move(callback), result = std::move(result)]() mutable {
                if (guard)
                    callback(std::move(result));
            },
            Qt::QueuedConnection);
    });
}

void Library::save(storage::SaveRequest request, QObject *context, std::function<void()> done)
{
    QPointer<QObject> guard(context);
    const bool firstPageChanged = !request.pageOrder.empty()
        && std::ranges::any_of(
            request.pages, [&](const core::Page &p) { return p.id == request.pageOrder.front(); });
    onIoThread([this, store = m_store, request = std::move(request), firstPageChanged, guard,
                done = std::move(done)]() mutable {
        auto saved = store->save(request);
        if (saved && firstPageChanged) {
            const auto first = std::ranges::find_if(request.pages, [&](const core::Page &p) {
                return p.id == request.pageOrder.front();
            });
            saved = store->saveThumbnail(request.info.id, renderThumbnail(*first));
        }
        QMetaObject::invokeMethod(
            this,
            [this, saved, info = request.info, guard, done = std::move(done)] {
                if (saved)
                    noteInfoChanged(info);
                else
                    emit storageError(saved.error().detail);
                if (guard && done)
                    done();
            },
            Qt::QueuedConnection);
    });
}
