// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/command.h"
#include "tratto/core/document.h"

#include <QColor>
#include <QElapsedTimer>
#include <QObject>
#include <QRectF>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <memory>

// The note open in the editor: its document, undo history and autosave.
// All content changes go through push() (a Command), so undo/redo and autosave
// stay in sync (ARCHITECTURE §4, §8).
class NoteSession : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged FINAL)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged FINAL)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged FINAL)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pagesChanged FINAL)
    // Paper of each page, for the pages panel: maps with pattern, spacingMm, color.
    Q_PROPERTY(QVariantList pages READ pages NOTIFY pagesChanged FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY infoChanged FINAL)
    Q_PROPERTY(QString groupId READ groupId NOTIFY infoChanged FINAL)
    // True while there are changes not yet on disk.
    Q_PROPERTY(bool saving READ saving NOTIFY savingChanged FINAL)

public:
    explicit NoteSession(QObject *parent = nullptr);
    ~NoteSession() override;

    // Loads a note from the Library (asynchronously; `loaded` turns true).
    Q_INVOKABLE void open(const QString &noteId);
    // A note that is never saved: for tests.
    void createBlank(int pattern, qreal spacingMm, const QColor &paperColor, int pages);

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    // Appends a page with the note's default paper.
    Q_INVOKABLE void addPage();
    // Saves pending changes now (leaving the editor, closing, suspending).
    Q_INVOKABLE void flush();

    bool loaded() const { return m_loaded; }
    bool canUndo() const { return m_history.canUndo(); }
    bool canRedo() const { return m_history.canRedo(); }
    int pageCount() const { return static_cast<int>(m_document.pages.size()); }
    QVariantList pages() const;
    QString title() const { return m_document.info.title; }
    void setTitle(const QString &title);
    QString groupId() const;
    bool saving() const { return m_document.isDirty() || m_inFlight > 0; }

    const tratto::core::Page *page(int index) const;
    int pageIndex(tratto::core::PageId id) const;

    void push(std::unique_ptr<tratto::core::Command> command);
    void beginGroup();
    void endGroup();

signals:
    // `bounds` in page pt; empty means the whole page.
    void pageChanged(int pageIndex, QRectF bounds);
    void loadedChanged();
    void historyChanged();
    void pagesChanged();
    void infoChanged();
    void savingChanged();
    void loadFailed();

private:
    void notify(const tratto::core::Change &change);
    void contentChanged();
    void saveNow();

    tratto::core::Document m_document;
    tratto::core::UndoStack m_history;
    bool m_loaded = false;
    bool m_persistent = false; // false for createBlank() documents
    int m_inFlight = 0;

    // Autosave: 500 ms after the last change, and at least every 5 s while writing.
    QTimer m_debounce;
    QElapsedTimer m_unsavedSince;
};
