// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/document.h"
#include "tratto/storage/note_store.h"

#include <QColor>
#include <QHash>
#include <QObject>
#include <QThread>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <functional>
#include <memory>

class QQmlEngine;
class QJSEngine;

// The note library, exposed to QML as the `Library` singleton.
//
// Holds the metadata of every note and the groups, and runs all disk work on a
// dedicated I/O thread (ARCHITECTURE §8): the GUI thread never waits for the disk.
// Requests run on the I/O thread in the order they were made.
class Library : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged FINAL)
    // Every note, trashed ones included: maps with id, title, group, created,
    // modified, inTrash, deleted, pageCount, paper { pattern, spacingMm, color }, thumbnail.
    Q_PROPERTY(QVariantList notes READ notes NOTIFY notesChanged FINAL)
    // Groups in display order: maps with id, name, color.
    Q_PROPERTY(QVariantList groups READ groups NOTIFY groupsChanged FINAL)
    Q_PROPERTY(QList<QColor> groupColors READ groupColors CONSTANT FINAL)

public:
    // Not default-constructible on purpose: QML must use create() and share the
    // instance owned by main(), not construct its own.
    Library(QString dataDir, QObject *parent);
    ~Library() override;

    static Library *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static void setInstance(Library *instance);
    static Library *instance();

    bool ready() const { return m_ready; }
    QVariantList notes() const { return m_notesCache; }
    QVariantList groups() const;
    QList<QColor> groupColors() const;

    // Creates and saves a one-page note; returns its id.
    Q_INVOKABLE QString createNote(const QString &title, const QString &groupId, int pattern,
                                   qreal spacingMm, const QColor &paperColor, int format,
                                   int orientation);
    Q_INVOKABLE void renameNote(const QString &id, const QString &title);
    Q_INVOKABLE void moveNote(const QString &id, const QString &groupId); // "" = standalone
    Q_INVOKABLE void trashNote(const QString &id);
    Q_INVOKABLE void restoreNote(const QString &id);
    Q_INVOKABLE void deleteNoteForever(const QString &id);
    Q_INVOKABLE void emptyTrash();

    Q_INVOKABLE QString createGroup(const QString &name, const QColor &color);
    Q_INVOKABLE void renameGroup(const QString &id, const QString &name);
    // The group's notes become standalone notes.
    Q_INVOKABLE void deleteGroup(const QString &id);
    Q_INVOKABLE QVariantMap group(const QString &id) const;

    // For NoteSession (C++ only).
    using LoadCallback = std::function<void(tratto::storage::Result<tratto::core::Document>)>;
    void load(tratto::core::NoteId id, QObject *context, LoadCallback callback);
    // Saves on the I/O thread, then calls `done` (on the GUI thread, if `context` is alive).
    void save(tratto::storage::SaveRequest request, QObject *context, std::function<void()> done);
    // Updates the cached metadata after a save from the editor.
    void noteInfoChanged(const tratto::core::NoteInfo &info);

signals:
    void readyChanged();
    void notesChanged();
    void groupsChanged();
    // A save failed; the editor keeps the changes and retries on the next save.
    void storageError(const QString &detail);

private:
    struct Entry
    {
        tratto::core::NoteInfo info;
        int thumbnailRevision = 0;
    };

    template<typename F> void onIoThread(F &&work);
    void refreshNotesCache();
    void updateInfo(const QString &id, const std::function<void(tratto::core::NoteInfo &)> &change);
    void saveGroups();
    QVariantMap toVariant(const Entry &entry) const;

    std::shared_ptr<tratto::storage::NoteStore> m_store; // used on the I/O thread only
    QThread m_ioThread;
    QObject *m_io = nullptr; // lives on m_ioThread; target for queued work

    bool m_ready = false;
    QHash<QString, Entry> m_entries;
    std::vector<tratto::core::Group> m_groups;
    QVariantList m_notesCache;
};
