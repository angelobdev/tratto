// SPDX-License-Identifier: GPL-3.0-or-later
#include "notesession.h"

#include "library.h"
#include "tratto/storage/note_store.h"

#include <QGuiApplication>
#include <QLoggingCategory>

#include <algorithm>

Q_LOGGING_CATEGORY(lcSession, "tratto.session")

namespace core = tratto::core;

namespace {

constexpr int kSaveDelayMs = 500;
constexpr qint64 kMaxUnsavedMs = 5000;

QColor toQColor(core::Rgba c)
{
    return QColor(c.r, c.g, c.b);
}

} // namespace

NoteSession::NoteSession(QObject *parent)
    : QObject(parent)
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(kSaveDelayMs);
    connect(&m_debounce, &QTimer::timeout, this, &NoteSession::saveNow);

    // Suspend, lock screen, switching away: save at once.
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
            [this](Qt::ApplicationState state) {
                if (state != Qt::ApplicationActive)
                    flush();
            });
}

NoteSession::~NoteSession()
{
    flush(); // the Library outlives sessions and finishes queued saves before exiting
}

void NoteSession::open(const QString &noteId)
{
    Library *library = Library::instance();
    if (!library)
        return;
    library->load(core::NoteId { QUuid::fromString(noteId) }, this,
                  [this](tratto::storage::Result<core::Document> result) {
                      if (!result) {
                          qCWarning(lcSession) << "Could not open note:" << result.error().detail;
                          emit loadFailed();
                          return;
                      }
                      m_document = std::move(*result);
                      m_history.clear();
                      m_persistent = true;
                      m_loaded = true;
                      emit loadedChanged();
                      emit infoChanged();
                      emit pagesChanged();
                      emit historyChanged();
                      for (int i = 0; i < pageCount(); ++i)
                          emit pageChanged(i, { });
                  });
}

void NoteSession::createBlank(int pattern, qreal spacingMm, const QColor &paperColor, int pages)
{
    core::PaperStyle paper;
    paper.background = { static_cast<core::PatternKind>(std::clamp(pattern, 0, 3)),
                         static_cast<float>(spacingMm) };
    const QColor rgb = paperColor.toRgb();
    paper.paperColor
        = { static_cast<std::uint8_t>(rgb.red()), static_cast<std::uint8_t>(rgb.green()),
            static_cast<std::uint8_t>(rgb.blue()), 255 };

    m_document = { };
    m_document.info.id = core::NoteId::create();
    m_document.info.defaultPaper = paper;
    for (int i = 0; i < std::max(pages, 1); ++i)
        m_document.pages.push_back(core::Page::create(paper));
    m_history.clear();
    m_persistent = false;
    m_loaded = true;

    emit loadedChanged();
    emit pagesChanged();
    emit historyChanged();
    for (int i = 0; i < pageCount(); ++i)
        emit pageChanged(i, { });
}

QVariantList NoteSession::pages() const
{
    QVariantList list;
    for (const core::Page &p : m_document.pages) {
        list.append(
            QVariantMap { { QStringLiteral("pattern"), static_cast<int>(p.paper.background.kind) },
                          { QStringLiteral("spacingMm"), p.paper.background.spacingMm },
                          { QStringLiteral("color"), toQColor(p.paper.paperColor) } });
    }
    return list;
}

QString NoteSession::groupId() const
{
    return m_document.info.group ? m_document.info.group->value.toString(QUuid::WithoutBraces)
                                 : QString();
}

void NoteSession::setTitle(const QString &title)
{
    const QString trimmed = title.trimmed();
    if (trimmed == m_document.info.title)
        return;
    m_document.info.title = trimmed;
    m_document.infoDirty = true;
    emit infoChanged();
    contentChanged();
}

const core::Page *NoteSession::page(int index) const
{
    if (index < 0 || index >= pageCount())
        return nullptr;
    return &m_document.pages[static_cast<std::size_t>(index)];
}

int NoteSession::pageIndex(core::PageId id) const
{
    const auto it
        = std::ranges::find_if(m_document.pages, [id](const core::Page &p) { return p.id == id; });
    return it == m_document.pages.end() ? -1 : static_cast<int>(it - m_document.pages.begin());
}

void NoteSession::push(std::unique_ptr<core::Command> command)
{
    notify(m_history.push(m_document, std::move(command)));
    if (!m_history.inGroup())
        emit historyChanged();
}

void NoteSession::beginGroup()
{
    m_history.beginGroup();
    emit historyChanged();
}

void NoteSession::endGroup()
{
    m_history.endGroup();
    emit historyChanged();
}

void NoteSession::undo()
{
    if (const auto change = m_history.undo(m_document)) {
        notify(*change);
        emit historyChanged();
    }
}

void NoteSession::redo()
{
    if (const auto change = m_history.redo(m_document)) {
        notify(*change);
        emit historyChanged();
    }
}

void NoteSession::addPage()
{
    if (!m_loaded)
        return;
    push(std::make_unique<core::AddPage>(core::Page::create(m_document.info.defaultPaper),
                                         m_document.pages.size()));
}

void NoteSession::notify(const core::Change &change)
{
    const int index = pageIndex(change.page);
    if (index >= 0)
        emit pageChanged(index, change.bounds);
    if (m_document.infoDirty)
        emit pagesChanged(); // page added or removed
    contentChanged();
}

void NoteSession::contentChanged()
{
    if (!m_persistent)
        return;
    if (!m_unsavedSince.isValid())
        m_unsavedSince.start();
    // Save 500 ms after the last change, but don't wait more than 5 s while writing.
    if (m_unsavedSince.elapsed() >= kMaxUnsavedMs)
        saveNow();
    else
        m_debounce.start();
    emit savingChanged();
}

void NoteSession::flush()
{
    if (m_persistent && m_document.isDirty())
        saveNow();
}

void NoteSession::saveNow()
{
    m_debounce.stop();
    m_unsavedSince.invalidate();
    Library *library = Library::instance();
    if (!m_persistent || !library || !m_document.isDirty())
        return;
    // Undo can rewind a change, but the note was still touched now.
    m_document.info.modified = QDateTime::currentDateTimeUtc();
    ++m_inFlight;
    library->save(tratto::storage::takeSaveRequest(m_document), this, [this] {
        --m_inFlight;
        emit savingChanged();
    });
    emit savingChanged();
}
