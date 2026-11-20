// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tratto/core/command.h"
#include "tratto/core/document.h"

#include <QColor>
#include <QObject>
#include <QRectF>
#include <QtQml/qqmlregistration.h>

#include <memory>

// The note open in the editor: its document and undo history.
// All changes go through push() (a Command), so undo/redo and autosave stay in sync.
class NoteSession : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged FINAL)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged FINAL)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pagesChanged FINAL)

public:
    explicit NoteSession(QObject *parent = nullptr);

    // Placeholder until notes are stored (M2): `pages` blank pages of the given paper.
    // `pattern` is a PaperThumbnail::Pattern value.
    Q_INVOKABLE void createBlank(int pattern, qreal spacingMm, const QColor &paperColor, int pages);

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    bool canUndo() const { return m_history.canUndo(); }
    bool canRedo() const { return m_history.canRedo(); }
    int pageCount() const { return static_cast<int>(m_document.pages.size()); }

    const tratto::core::Page *page(int index) const;
    int pageIndex(tratto::core::PageId id) const;

    void push(std::unique_ptr<tratto::core::Command> command);
    void beginGroup();
    void endGroup();

signals:
    // `bounds` in page pt; empty means the whole page.
    void pageChanged(int pageIndex, QRectF bounds);
    void historyChanged();
    void pagesChanged();

private:
    void notify(const tratto::core::Change &change);

    tratto::core::Document m_document;
    tratto::core::UndoStack m_history;
};
