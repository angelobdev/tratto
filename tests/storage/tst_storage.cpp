// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/storage/note_store.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

using namespace tratto;
using storage::NoteStore;

namespace {

core::Stroke stroke(float y, core::StrokeTool tool = core::StrokeTool::Pen)
{
    core::Stroke s;
    s.id = core::ObjectId::create();
    s.tool = tool;
    s.color = core::Rgba::fromRgb(0xE03131);
    s.width = 2.0F;
    s.points = { { 10, y, 0.2F }, { 50, y + 5, 0.9F }, { 90, y, 0.5F } };
    s.updateBounds();
    return s;
}

core::Document makeNote(int pages)
{
    core::Document doc;
    doc.info.id = core::NoteId::create();
    doc.info.title = QStringLiteral("Analisi II — integrali");
    doc.info.group = core::GroupId::create();
    doc.info.created = QDateTime(QDate(2026, 11, 19), QTime(16, 42), QTimeZone::UTC);
    doc.info.modified = doc.info.created.addSecs(60);
    doc.info.defaultPaper.background = { core::PatternKind::Grid, 5.0F };
    doc.info.defaultPaper.paperColor = core::Rgba::fromRgb(0xFBF6EA);
    for (int i = 0; i < pages; ++i) {
        doc.pages.push_back(core::Page::create(doc.info.defaultPaper));
        doc.pages.back().objects.emplace_back(stroke(100.0F + 10.0F * static_cast<float>(i)));
    }
    return doc;
}

void compareStrokes(const core::Stroke &a, const core::Stroke &b)
{
    QCOMPARE(a.id, b.id);
    QCOMPARE(a.tool, b.tool);
    QCOMPARE(a.color, b.color);
    QCOMPARE(a.width, b.width);
    QCOMPARE(a.points, b.points); // float32 on disk: exact for float input
    QCOMPARE(a.bbox, b.bbox);
}

} // namespace

class TestStorage : public QObject
{
    Q_OBJECT

private slots:
    void manifestRoundTrip()
    {
        storage::Manifest m;
        m.info = makeNote(0).info;
        m.info.deleted = m.info.modified.addDays(1);
        m.pages = { core::PageId::create(), core::PageId::create() };

        const auto decoded = storage::decodeManifest(storage::encodeManifest(m));
        QVERIFY(decoded.has_value());
        QCOMPARE(decoded->info.id, m.info.id);
        QCOMPARE(decoded->info.title, m.info.title);
        QCOMPARE(decoded->info.group, m.info.group);
        QCOMPARE(decoded->info.created, m.info.created);
        QCOMPARE(decoded->info.modified, m.info.modified);
        QCOMPARE(decoded->info.deleted, m.info.deleted);
        QCOMPARE(decoded->info.defaultPaper, m.info.defaultPaper);
        QCOMPARE(decoded->pages, m.pages);
        QCOMPARE(decoded->info.pageCount, 2);
    }

    void standaloneNoteHasNoGroup()
    {
        storage::Manifest m;
        m.info = makeNote(0).info;
        m.info.group.reset();
        const auto decoded = storage::decodeManifest(storage::encodeManifest(m));
        QVERIFY(decoded.has_value());
        QVERIFY(!decoded->info.group.has_value());
        QVERIFY(!decoded->info.inTrash());
    }

    void newerFormatIsRejected()
    {
        QJsonObject o
            = QJsonDocument::fromJson(storage::encodeManifest({ makeNote(0).info, { } })).object();
        o.insert(QStringLiteral("format_version"), storage::kFormatVersion + 1);
        const auto decoded = storage::decodeManifest(QJsonDocument(o).toJson());
        QVERIFY(!decoded.has_value());
        QCOMPARE(decoded.error().code, storage::ErrorCode::NewerVersion);
    }

    void garbageIsCorrupt()
    {
        QCOMPARE(storage::decodeManifest("{not json").error().code, storage::ErrorCode::Corrupt);
        QCOMPARE(storage::decodePage("\xff\x00garbage").error().code, storage::ErrorCode::Corrupt);
    }

    void pageRoundTrip()
    {
        core::Page page = core::Page::create({ });
        page.objects.emplace_back(stroke(100));
        page.objects.emplace_back(stroke(200, core::StrokeTool::Highlighter));

        const auto decoded = storage::decodePage(storage::encodePage(page));
        QVERIFY(decoded.has_value());
        QCOMPARE(decoded->id, page.id);
        QCOMPARE(decoded->size, page.size);
        QCOMPARE(decoded->paper, page.paper);
        QCOMPARE(decoded->objects.size(), 2u);
        for (std::size_t i = 0; i < 2; ++i)
            compareStrokes(std::get<core::Stroke>(decoded->objects[i]),
                           std::get<core::Stroke>(page.objects[i]));
    }

    void textBoxRoundTrip()
    {
        core::Page page = core::Page::create({ });
        core::TextBox t;
        t.id = core::ObjectId::create();
        t.bbox = QRectF(50, 400, 300, 48);
        t.text = QStringLiteral("Ricontrollare l'esercizio 4\nprima dell'esame — ∬ f dA");
        t.font = core::TextFont::Serif;
        t.size = 20.0F;
        t.bold = true;
        t.italic = true;
        t.color = core::Rgba::fromRgb(0x1E3A8A);
        page.objects.emplace_back(stroke(100));
        page.objects.emplace_back(t);

        const auto decoded = storage::decodePage(storage::encodePage(page));
        QVERIFY(decoded.has_value());
        QCOMPARE(decoded->objects.size(), 2u);
        const auto &d = std::get<core::TextBox>(decoded->objects[1]);
        QCOMPARE(d.id, t.id);
        QCOMPARE(d.bbox, t.bbox);
        QCOMPARE(d.text, t.text);
        QCOMPARE(d.font, t.font);
        QCOMPARE(d.size, t.size);
        QCOMPARE(d.bold, true);
        QCOMPARE(d.italic, true);
        QCOMPARE(d.color, t.color);
    }

    void version1ManifestIsMigrated()
    {
        // A manifest written by format_version 1 (before text boxes).
        const QByteArray v1 = R"({
            "format_version": 1,
            "id": "01a1f877-22e7-77f2-bdd8-052df6b0cc37",
            "title": "Old note",
            "group": null,
            "created": "2026-11-19T10:00:00.000Z",
            "modified": "2026-11-19T11:00:00.000Z",
            "deleted": null,
            "default_paper": { "pattern": "ruled", "spacing_mm": 6, "color": "#FFFFFF",
                               "format": "a4", "orientation": "portrait" },
            "pages": ["01a1f877-22e7-77f2-bdd8-052df6b0cc38"]
        })";
        const auto migrated = storage::migrateManifest(QJsonDocument::fromJson(v1).object());
        QVERIFY(migrated.has_value());
        QCOMPARE(migrated->value(QStringLiteral("format_version")).toInt(),
                 storage::kFormatVersion);
        const auto decoded = storage::decodeManifest(v1);
        QVERIFY(decoded.has_value());
        QCOMPARE(decoded->info.title, QStringLiteral("Old note"));
        QCOMPARE(decoded->pages.size(), 1u);
        // Saved again, it is written with the current version.
        QVERIFY(storage::encodeManifest(*decoded).contains("\"format_version\": 2"));
    }

    void groupsRoundTrip()
    {
        const std::vector<core::Group> groups = {
            { core::GroupId::create(), QStringLiteral("Università"), core::Rgba::fromRgb(0xAE3EC9),
              0 },
            { core::GroupId::create(), QStringLiteral("Lavoro"), core::Rgba::fromRgb(0xE8590C), 1 },
        };
        const auto decoded = storage::decodeGroups(storage::encodeGroups(groups));
        QVERIFY(decoded.has_value());
        QCOMPARE(decoded->size(), 2u);
        QCOMPARE((*decoded)[0].name, groups[0].name);
        QCOMPARE((*decoded)[1].color, groups[1].color);
        QCOMPARE((*decoded)[1].id, groups[1].id);
    }

    void saveAndLoadNote()
    {
        QTemporaryDir dir;
        NoteStore store(dir.path());
        core::Document doc = makeNote(3);
        const auto request = storage::takeSaveRequest(doc, true);
        QVERIFY(!doc.isDirty());
        QVERIFY(store.save(request).has_value());

        const auto loaded = store.load(doc.info.id);
        QVERIFY(loaded.has_value());
        QCOMPARE(loaded->info.title, doc.info.title);
        QCOMPARE(loaded->info.pageCount, 3);
        QCOMPARE(loaded->pages.size(), 3u);
        for (std::size_t i = 0; i < 3; ++i) {
            QCOMPARE(loaded->pages[i].id, doc.pages[i].id);
            compareStrokes(std::get<core::Stroke>(loaded->pages[i].objects[0]),
                           std::get<core::Stroke>(doc.pages[i].objects[0]));
        }
    }

    void onlyDirtyPagesAreWritten()
    {
        QTemporaryDir dir;
        NoteStore store(dir.path());
        core::Document doc = makeNote(2);
        QVERIFY(store.save(storage::takeSaveRequest(doc, true)).has_value());

        doc.pages[1].objects.emplace_back(stroke(300));
        doc.dirtyPages.insert(doc.pages[1].id);
        const auto request = storage::takeSaveRequest(doc);
        QCOMPARE(request.pages.size(), 1u);
        QCOMPARE(request.pages[0].id, doc.pages[1].id);
        QVERIFY(store.save(request).has_value());
        QCOMPARE(store.load(doc.info.id)->pages[1].objects.size(), 2u);
    }

    void removedPagesLoseTheirFiles()
    {
        QTemporaryDir dir;
        NoteStore store(dir.path());
        core::Document doc = makeNote(3);
        QVERIFY(store.save(storage::takeSaveRequest(doc, true)).has_value());

        doc.pages.erase(doc.pages.begin() + 1);
        doc.infoDirty = true;
        QVERIFY(store.save(storage::takeSaveRequest(doc)).has_value());

        const QDir pages(dir.path() + QStringLiteral("/notes/")
                         + storage::idToString(doc.info.id.value) + QStringLiteral("/pages"));
        QCOMPARE(pages.entryList({ QStringLiteral("*.cbor") }, QDir::Files).size(), 2);
        QCOMPARE(store.load(doc.info.id)->pages.size(), 2u);
    }

    void updateInfoKeepsPages()
    {
        QTemporaryDir dir;
        NoteStore store(dir.path());
        core::Document doc = makeNote(2);
        QVERIFY(store.save(storage::takeSaveRequest(doc, true)).has_value());

        core::NoteInfo info = doc.info;
        info.title = QStringLiteral("Renamed");
        info.deleted = QDateTime::currentDateTimeUtc();
        QVERIFY(store.updateInfo(info).has_value());

        const auto loaded = store.load(doc.info.id);
        QCOMPARE(loaded->info.title, QStringLiteral("Renamed"));
        QVERIFY(loaded->info.inTrash());
        QCOMPARE(loaded->pages.size(), 2u);
    }

    void listSkipsBrokenBundles()
    {
        QTemporaryDir dir;
        NoteStore store(dir.path());
        core::Document a = makeNote(1);
        core::Document b = makeNote(1);
        QVERIFY(store.save(storage::takeSaveRequest(a, true)).has_value());
        QVERIFY(store.save(storage::takeSaveRequest(b, true)).has_value());

        QFile broken(dir.path() + QStringLiteral("/notes/") + storage::idToString(b.info.id.value)
                     + QStringLiteral("/manifest.json"));
        QVERIFY(broken.open(QIODevice::WriteOnly | QIODevice::Truncate));
        broken.write("{ half a manifest");
        broken.close();

        const auto notes = store.list();
        QCOMPARE(notes.size(), 1u);
        QCOMPARE(notes[0].id, a.info.id);
    }

    void interruptedWriteLeavesTheNoteLoadable()
    {
        QTemporaryDir dir;
        NoteStore store(dir.path());
        core::Document doc = makeNote(1);
        QVERIFY(store.save(storage::takeSaveRequest(doc, true)).has_value());

        // What a crash during QSaveFile leaves behind: a half-written temporary file.
        const QString noteDir
            = dir.path() + QStringLiteral("/notes/") + storage::idToString(doc.info.id.value);
        QFile leftover(noteDir + QStringLiteral("/manifest.json.a1B2c3"));
        QVERIFY(leftover.open(QIODevice::WriteOnly));
        leftover.write("{ \"format_version\": 1, \"ti");
        leftover.close();

        QVERIFY(store.load(doc.info.id).has_value());
        QCOMPARE(store.removeTemporaryFiles(), 1);
        QVERIFY(!leftover.exists());
        QVERIFY(QFile::exists(noteDir + QStringLiteral("/manifest.json")));
    }

    void trashIsPurgedAfter30Days()
    {
        QTemporaryDir dir;
        NoteStore store(dir.path());
        const QDateTime now = QDateTime::currentDateTimeUtc();
        core::Document old = makeNote(1);
        old.info.deleted = now.addDays(-31);
        core::Document recent = makeNote(1);
        recent.info.deleted = now.addDays(-2);
        core::Document kept = makeNote(1);
        for (core::Document *doc : { &old, &recent, &kept })
            QVERIFY(store.save(storage::takeSaveRequest(*doc, true)).has_value());

        QCOMPARE(store.purgeTrash(now), 1);
        QCOMPARE(store.list().size(), 2u);
        QCOMPARE(store.load(old.info.id).error().code, storage::ErrorCode::NotFound);
    }

    void thumbnailAndGroupsOnDisk()
    {
        QTemporaryDir dir;
        NoteStore store(dir.path());
        core::Document doc = makeNote(1);
        QVERIFY(store.save(storage::takeSaveRequest(doc, true)).has_value());

        QImage thumb(40, 56, QImage::Format_RGB32);
        thumb.fill(Qt::white);
        QVERIFY(store.saveThumbnail(doc.info.id, thumb).has_value());
        QCOMPARE(QImage(store.thumbnailPath(doc.info.id)).size(), QSize(40, 56));

        QVERIFY(store.loadGroups()->empty()); // no file yet
        QVERIFY(store
                    .saveGroups({ { core::GroupId::create(), QStringLiteral("Casa"),
                                    core::Rgba::fromRgb(0x0CA678), 0 } })
                    .has_value());
        QCOMPARE(store.loadGroups()->size(), 1u);
    }
};

QTEST_MAIN(TestStorage)
#include "tst_storage.moc"
