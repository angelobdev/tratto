// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/storage/format.h"

#include <QCborArray>
#include <QCborMap>
#include <QCborValue>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtEndian>

#include <array>

namespace tratto::storage {

namespace {

using core::Orientation;
using core::PageFormat;
using core::PatternKind;

template<typename Enum, std::size_t N>
QString enumToString(Enum value, const std::array<std::pair<Enum, const char *>, N> &names)
{
    for (const auto &[e, name] : names) {
        if (e == value)
            return QString::fromLatin1(name);
    }
    return QString::fromLatin1(names.front().second);
}

template<typename Enum, std::size_t N>
Enum enumFromString(const QString &text, const std::array<std::pair<Enum, const char *>, N> &names)
{
    for (const auto &[e, name] : names) {
        if (text == QLatin1String(name))
            return e;
    }
    return names.front().first;
}

constexpr std::array<std::pair<PatternKind, const char *>, 4> kPatterns = { {
    { PatternKind::Blank, "blank" },
    { PatternKind::Ruled, "ruled" },
    { PatternKind::Grid, "grid" },
    { PatternKind::Dots, "dots" },
} };
constexpr std::array<std::pair<PageFormat, const char *>, 3> kFormats = { {
    { PageFormat::A4, "a4" },
    { PageFormat::Letter, "letter" },
    { PageFormat::Continuous, "continuous" },
} };
constexpr std::array<std::pair<Orientation, const char *>, 2> kOrientations = { {
    { Orientation::Portrait, "portrait" },
    { Orientation::Landscape, "landscape" },
} };
constexpr std::array<std::pair<core::TextFont, const char *>, 3> kFonts = { {
    { core::TextFont::Sans, "sans" },
    { core::TextFont::Serif, "serif" },
    { core::TextFont::Monospace, "monospace" },
} };
constexpr std::array<std::pair<core::StrokeTool, const char *>, 2> kTools = { {
    { core::StrokeTool::Pen, "pen" },
    { core::StrokeTool::Highlighter, "highlighter" },
} };

QString colorToString(core::Rgba c)
{
    return QStringLiteral("#%1%2%3")
        .arg(c.r, 2, 16, QLatin1Char('0'))
        .arg(c.g, 2, 16, QLatin1Char('0'))
        .arg(c.b, 2, 16, QLatin1Char('0'))
        .toUpper();
}

core::Rgba colorFromString(const QString &text)
{
    bool ok = false;
    const uint rgb = text.mid(1).toUInt(&ok, 16);
    return ok && text.size() == 7 ? core::Rgba::fromRgb(rgb) : core::kWhite;
}

QUuid uuidFrom(const QString &text)
{
    return QUuid::fromString(text);
}

QString dateToString(const QDateTime &date)
{
    return date.toUTC().toString(Qt::ISODateWithMs);
}

QDateTime dateFromString(const QString &text)
{
    return QDateTime::fromString(text, Qt::ISODateWithMs);
}

QJsonObject paperToJson(const core::PaperStyle &p)
{
    return {
        { QStringLiteral("pattern"), enumToString(p.background.kind, kPatterns) },
        { QStringLiteral("spacing_mm"), static_cast<double>(p.background.spacingMm) },
        { QStringLiteral("color"), colorToString(p.paperColor) },
        { QStringLiteral("format"), enumToString(p.format, kFormats) },
        { QStringLiteral("orientation"), enumToString(p.orientation, kOrientations) },
    };
}

core::PaperStyle paperFromJson(const QJsonObject &o)
{
    core::PaperStyle p;
    p.background.kind = enumFromString(o.value(QStringLiteral("pattern")).toString(), kPatterns);
    p.background.spacingMm = static_cast<float>(o.value(QStringLiteral("spacing_mm")).toDouble());
    p.paperColor = colorFromString(o.value(QStringLiteral("color")).toString());
    p.format = enumFromString(o.value(QStringLiteral("format")).toString(), kFormats);
    p.orientation
        = enumFromString(o.value(QStringLiteral("orientation")).toString(), kOrientations);
    return p;
}

// Stroke points: little-endian float32 triples (x, y, pressure).
QByteArray pointsToBytes(const std::vector<core::StrokePoint> &points)
{
    QByteArray bytes(static_cast<qsizetype>(points.size() * 3 * sizeof(float)), Qt::Uninitialized);
    auto *out = reinterpret_cast<uchar *>(bytes.data());
    for (const core::StrokePoint &p : points) {
        for (const float v : { p.x, p.y, p.pressure }) {
            qToLittleEndian(v, out);
            out += sizeof(float);
        }
    }
    return bytes;
}

std::vector<core::StrokePoint> pointsFromBytes(const QByteArray &bytes)
{
    std::vector<core::StrokePoint> points;
    const auto count = static_cast<std::size_t>(bytes.size()) / (3 * sizeof(float));
    points.reserve(count);
    const auto *in = reinterpret_cast<const uchar *>(bytes.constData());
    for (std::size_t i = 0; i < count; ++i) {
        core::StrokePoint p;
        p.x = qFromLittleEndian<float>(in);
        p.y = qFromLittleEndian<float>(in + sizeof(float));
        p.pressure = qFromLittleEndian<float>(in + 2 * sizeof(float));
        in += 3 * sizeof(float);
        points.push_back(p);
    }
    return points;
}

Error corrupt(const QString &detail)
{
    return { ErrorCode::Corrupt, detail };
}

} // namespace

QString idToString(const QUuid &id)
{
    return id.toString(QUuid::WithoutBraces);
}

QByteArray encodeManifest(const Manifest &m)
{
    QJsonArray pages;
    for (const core::PageId &id : m.pages)
        pages.append(idToString(id.value));
    QJsonObject o {
        { QStringLiteral("format_version"), kFormatVersion },
        { QStringLiteral("id"), idToString(m.info.id.value) },
        { QStringLiteral("title"), m.info.title },
        { QStringLiteral("group"),
          m.info.group ? QJsonValue(idToString(m.info.group->value)) : QJsonValue() },
        { QStringLiteral("created"), dateToString(m.info.created) },
        { QStringLiteral("modified"), dateToString(m.info.modified) },
        { QStringLiteral("deleted"),
          m.info.deleted ? QJsonValue(dateToString(*m.info.deleted)) : QJsonValue() },
        { QStringLiteral("default_paper"), paperToJson(m.info.defaultPaper) },
        { QStringLiteral("pages"), pages },
    };
    return QJsonDocument(o).toJson(QJsonDocument::Indented);
}

Result<QJsonObject> migrateManifest(QJsonObject manifest)
{
    int version = manifest.value(QStringLiteral("format_version")).toInt(0);
    if (version > kFormatVersion)
        return std::unexpected(
            Error { ErrorCode::NewerVersion, QStringLiteral("format_version %1").arg(version) });
    if (version < 1)
        return std::unexpected(corrupt(QStringLiteral("missing format_version")));
    // One step per version.
    if (version == 1) {
        // 2 adds text boxes to pages; version 1 pages have none, so nothing changes.
        version = 2;
    }
    manifest.insert(QStringLiteral("format_version"), kFormatVersion);
    return manifest;
}

Result<Manifest> decodeManifest(const QByteArray &json)
{
    QJsonParseError parseError { };
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (!doc.isObject())
        return std::unexpected(corrupt(parseError.errorString()));
    const auto migrated = migrateManifest(doc.object());
    if (!migrated)
        return std::unexpected(migrated.error());
    const QJsonObject &o = *migrated;

    Manifest m;
    m.info.id = { uuidFrom(o.value(QStringLiteral("id")).toString()) };
    if (m.info.id.isNull())
        return std::unexpected(corrupt(QStringLiteral("missing note id")));
    m.info.title = o.value(QStringLiteral("title")).toString();
    if (const QString group = o.value(QStringLiteral("group")).toString(); !group.isEmpty())
        m.info.group = core::GroupId { uuidFrom(group) };
    m.info.created = dateFromString(o.value(QStringLiteral("created")).toString());
    m.info.modified = dateFromString(o.value(QStringLiteral("modified")).toString());
    if (const QString deleted = o.value(QStringLiteral("deleted")).toString(); !deleted.isEmpty())
        m.info.deleted = dateFromString(deleted);
    m.info.defaultPaper = paperFromJson(o.value(QStringLiteral("default_paper")).toObject());
    for (const QJsonValue &page : o.value(QStringLiteral("pages")).toArray()) {
        const core::PageId id { uuidFrom(page.toString()) };
        if (id.isNull())
            return std::unexpected(corrupt(QStringLiteral("bad page id")));
        m.pages.push_back(id);
    }
    m.info.pageCount = static_cast<int>(m.pages.size());
    return m;
}

QByteArray encodePage(const core::Page &page)
{
    QCborArray objects;
    for (const core::Object &object : page.objects) {
        if (const auto *s = std::get_if<core::Stroke>(&object)) {
            objects.append(QCborMap {
                { QStringLiteral("type"), QStringLiteral("stroke") },
                { QStringLiteral("id"), idToString(s->id.value) },
                { QStringLiteral("tool"), enumToString(s->tool, kTools) },
                { QStringLiteral("color"), colorToString(s->color) },
                { QStringLiteral("width"), static_cast<double>(s->width) },
                { QStringLiteral("points"), pointsToBytes(s->points) },
            });
        } else if (const auto *t = std::get_if<core::TextBox>(&object)) {
            objects.append(QCborMap {
                { QStringLiteral("type"), QStringLiteral("text") },
                { QStringLiteral("id"), idToString(t->id.value) },
                { QStringLiteral("x"), t->bbox.x() },
                { QStringLiteral("y"), t->bbox.y() },
                { QStringLiteral("width"), t->bbox.width() },
                { QStringLiteral("height"), t->bbox.height() },
                { QStringLiteral("text"), t->text },
                { QStringLiteral("font"), enumToString(t->font, kFonts) },
                { QStringLiteral("size"), static_cast<double>(t->size) },
                { QStringLiteral("bold"), t->bold },
                { QStringLiteral("italic"), t->italic },
                { QStringLiteral("color"), colorToString(t->color) },
            });
        }
    }
    const QCborMap map {
        { QStringLiteral("id"), idToString(page.id.value) },
        { QStringLiteral("width"), page.size.width() },
        { QStringLiteral("height"), page.size.height() },
        { QStringLiteral("paper"), QCborValue::fromJsonValue(paperToJson(page.paper)) },
        { QStringLiteral("objects"), objects },
    };
    return QCborValue(map).toCbor();
}

Result<core::Page> decodePage(const QByteArray &cbor)
{
    QCborParserError parseError { };
    const QCborValue value = QCborValue::fromCbor(cbor, &parseError);
    if (parseError.error != QCborError::NoError || !value.isMap())
        return std::unexpected(corrupt(parseError.errorString()));
    const QCborMap map = value.toMap();

    core::Page page;
    page.id = { uuidFrom(map.value(QStringLiteral("id")).toString()) };
    if (page.id.isNull())
        return std::unexpected(corrupt(QStringLiteral("missing page id")));
    page.size = QSizeF(map.value(QStringLiteral("width")).toDouble(),
                       map.value(QStringLiteral("height")).toDouble());
    page.paper = paperFromJson(map.value(QStringLiteral("paper")).toJsonValue().toObject());

    for (const QCborValue &item : map.value(QStringLiteral("objects")).toArray()) {
        const QCborMap o = item.toMap();
        const QString type = o.value(QStringLiteral("type")).toString();
        if (type == QLatin1String("text")) {
            core::TextBox t;
            t.id = { uuidFrom(o.value(QStringLiteral("id")).toString()) };
            t.bbox = QRectF(o.value(QStringLiteral("x")).toDouble(),
                            o.value(QStringLiteral("y")).toDouble(),
                            o.value(QStringLiteral("width")).toDouble(),
                            o.value(QStringLiteral("height")).toDouble());
            t.text = o.value(QStringLiteral("text")).toString();
            t.font = enumFromString(o.value(QStringLiteral("font")).toString(), kFonts);
            t.size = static_cast<float>(o.value(QStringLiteral("size")).toDouble(16.0));
            t.bold = o.value(QStringLiteral("bold")).toBool();
            t.italic = o.value(QStringLiteral("italic")).toBool();
            t.color = colorFromString(o.value(QStringLiteral("color")).toString());
            if (t.id.isNull())
                return std::unexpected(corrupt(QStringLiteral("bad text box")));
            page.objects.emplace_back(std::move(t));
            continue;
        }
        // Unknown object types (from a newer version) would be lost on save:
        // the manifest's format_version check prevents loading such notes.
        if (type != QLatin1String("stroke"))
            continue;
        core::Stroke s;
        s.id = { uuidFrom(o.value(QStringLiteral("id")).toString()) };
        s.tool = enumFromString(o.value(QStringLiteral("tool")).toString(), kTools);
        s.color = colorFromString(o.value(QStringLiteral("color")).toString());
        s.width = static_cast<float>(o.value(QStringLiteral("width")).toDouble());
        s.points = pointsFromBytes(o.value(QStringLiteral("points")).toByteArray());
        if (s.id.isNull() || s.points.empty())
            return std::unexpected(corrupt(QStringLiteral("bad stroke")));
        s.updateBounds();
        page.objects.emplace_back(std::move(s));
    }
    return page;
}

QByteArray encodeGroups(const std::vector<core::Group> &groups)
{
    QJsonArray list;
    for (const core::Group &g : groups) {
        list.append(QJsonObject {
            { QStringLiteral("id"), idToString(g.id.value) },
            { QStringLiteral("name"), g.name },
            { QStringLiteral("color"), colorToString(g.color) },
            { QStringLiteral("order"), g.order },
        });
    }
    const QJsonObject o { { QStringLiteral("format_version"), kFormatVersion },
                          { QStringLiteral("groups"), list } };
    return QJsonDocument(o).toJson(QJsonDocument::Indented);
}

Result<std::vector<core::Group>> decodeGroups(const QByteArray &json)
{
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject())
        return std::unexpected(corrupt(QStringLiteral("groups.json")));
    if (doc.object().value(QStringLiteral("format_version")).toInt() > kFormatVersion)
        return std::unexpected(Error { ErrorCode::NewerVersion, QStringLiteral("groups.json") });
    std::vector<core::Group> groups;
    for (const QJsonValue &v : doc.object().value(QStringLiteral("groups")).toArray()) {
        const QJsonObject o = v.toObject();
        core::Group g;
        g.id = { uuidFrom(o.value(QStringLiteral("id")).toString()) };
        g.name = o.value(QStringLiteral("name")).toString();
        g.color = colorFromString(o.value(QStringLiteral("color")).toString());
        g.order = o.value(QStringLiteral("order")).toInt();
        if (!g.id.isNull())
            groups.push_back(g);
    }
    return groups;
}

} // namespace tratto::storage
