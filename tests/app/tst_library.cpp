// SPDX-License-Identifier: GPL-3.0-or-later
// End to end: Library + NoteSession + NoteStore on a temporary folder.
// Close and reopen without losing anything (the M2 goal).
#include "library.h"
#include "notesession.h"
#include "tratto/core/command.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

namespace core = tratto::core;

namespace {

core::Stroke makeStroke(float y)
{
    core::Stroke s;
    s.id = core::ObjectId::create();
    s.points = { { 50, y, 0.5F }, { 150, y, 0.8F } };
    s.updateBounds();
    return s;
}

void addStroke(NoteSession &session, float y)
{
    session.push(std::make_unique<core::AddObjects>(session.page(0)->id,
                                                    std::vector<core::Object> { makeStroke(y) }));
}

QVariantMap findNote(const Library &library, const QString &id)
{
    for (const QVariant &v : library.notes()) {
        if (v.toMap().value(QStringLiteral("id")).toString() == id)
            return v.toMap();
    }
    return { };
}

std::unique_ptr<Library> openLibrary(const QString &dir)
{
    auto library = std::make_unique<Library>(dir, nullptr);
    Library::setInstance(library.get());
    return library;
}

} // namespace

class TestLibrary : public QObject
{
    Q_OBJECT

private slots:
    void writeCloseReopen()
    {
        QTemporaryDir dir;
        QString id;
        {
            auto library = openLibrary(dir.path());
            QTRY_VERIFY(library->ready());
            id = library->createNote(QStringLiteral("Physics"), { }, 1, 6, Qt::white, 0, 0);
            QCOMPARE(library->notes().size(), 1);

            NoteSession session;
            session.open(id);
            QTRY_VERIFY(session.loaded());
            QCOMPARE(session.title(), QStringLiteral("Physics"));
            QCOMPARE(session.pageCount(), 1);

            addStroke(session, 100);
            QVERIFY(session.saving());
            QTRY_VERIFY(!session.saving()); // autosave after 500 ms

            // The thumbnail of the first page was written and its URL changed.
            QVERIFY(findNote(*library, id)
                        .value(QStringLiteral("thumbnail"))
                        .toString()
                        .endsWith(QStringLiteral("?v=2")));
            session.setTitle(QStringLiteral("Physics — waves"));
            session.addPage();
            QTRY_VERIFY(!session.saving());
        }
        {
            auto library = openLibrary(dir.path());
            QTRY_VERIFY(library->ready());
            const QVariantMap note = findNote(*library, id);
            QCOMPARE(note.value(QStringLiteral("title")).toString(),
                     QStringLiteral("Physics — waves"));
            QCOMPARE(note.value(QStringLiteral("pageCount")).toInt(), 2);

            NoteSession session;
            session.open(id);
            QTRY_VERIFY(session.loaded());
            QCOMPARE(session.page(0)->objects.size(), 1u);
            QCOMPARE(session.pageCount(), 2);
        }
    }

    void lastChangesAreSavedOnClose()
    {
        QTemporaryDir dir;
        QString id;
        {
            auto library = openLibrary(dir.path());
            QTRY_VERIFY(library->ready());
            id = library->createNote(QStringLiteral("Quick"), { }, 0, 0, Qt::white, 0, 0);
            auto session = std::make_unique<NoteSession>();
            session->open(id);
            QTRY_VERIFY(session->loaded());
            addStroke(*session, 100);
            addStroke(*session, 200);
            // No waiting for autosave: closing the editor and the app right away.
            session.reset();
        }
        auto library = openLibrary(dir.path());
        QTRY_VERIFY(library->ready());
        NoteSession session;
        session.open(id);
        QTRY_VERIFY(session.loaded());
        QCOMPARE(session.page(0)->objects.size(), 2u);
    }

    void trashGroupsAndRenamesPersist()
    {
        QTemporaryDir dir;
        QString keep, trashed, gone, group;
        {
            auto library = openLibrary(dir.path());
            QTRY_VERIFY(library->ready());
            group = library->createGroup(QStringLiteral("Università"), QColor(0xAE3EC9));
            keep = library->createNote(QStringLiteral("A"), group, 0, 0, Qt::white, 0, 0);
            trashed = library->createNote(QStringLiteral("B"), group, 0, 0, Qt::white, 0, 0);
            gone = library->createNote(QStringLiteral("C"), { }, 0, 0, Qt::white, 0, 0);
            library->renameNote(keep, QStringLiteral("  A renamed  "));
            library->trashNote(trashed);
            library->trashNote(gone);
            library->deleteNoteForever(gone);
            library->renameGroup(group, QStringLiteral("Uni"));
        }
        {
            auto library = openLibrary(dir.path());
            QTRY_VERIFY(library->ready());
            QCOMPARE(library->notes().size(), 2);
            QCOMPARE(findNote(*library, keep).value(QStringLiteral("title")).toString(),
                     QStringLiteral("A renamed"));
            QVERIFY(findNote(*library, trashed).value(QStringLiteral("inTrash")).toBool());
            QCOMPARE(library->group(group).value(QStringLiteral("name")).toString(),
                     QStringLiteral("Uni"));

            // Deleting the group turns its notes into standalone notes.
            library->restoreNote(trashed);
            library->deleteGroup(group);
            QCOMPARE(findNote(*library, keep).value(QStringLiteral("group")).toString(), QString());
        }
        auto library = openLibrary(dir.path());
        QTRY_VERIFY(library->ready());
        QVERIFY(library->groups().isEmpty());
        QVERIFY(!findNote(*library, trashed).value(QStringLiteral("inTrash")).toBool());
        QCOMPARE(findNote(*library, trashed).value(QStringLiteral("group")).toString(), QString());

        library->emptyTrash(); // nothing in it
        QCOMPARE(library->notes().size(), 2);
    }
};

QTEST_MAIN(TestLibrary)
#include "tst_library.moc"
