// SPDX-License-Identifier: GPL-3.0-or-later
#include "preferences.h"

#include "appsettings.h"

#include <QJSEngine>
#include <QMetaEnum>

namespace {

Preferences *s_instance = nullptr;

const QString kTabletMode = QStringLiteral("input/tabletMode");
const QString kStylusOnly = QStringLiteral("input/stylusOnly");
const QString kBarrelButtonErases = QStringLiteral("input/barrelButtonErases");
const QString kTapToUndo = QStringLiteral("input/tapToUndo");
const QString kNewNotePaper = QStringLiteral("newNote/paper");

// White, ruled 6 mm, A4 portrait (DESIGN.md defaults). Pattern/format/orientation
// values follow the C++ enums (PaperThumbnail::Pattern, PageFormat, Orientation).
QVariantMap defaultPaper()
{
    return { { QStringLiteral("pattern"), 1 },
             { QStringLiteral("spacingMm"), 6.0 },
             { QStringLiteral("color"), QStringLiteral("#FFFFFF") },
             { QStringLiteral("format"), 0 },
             { QStringLiteral("orientation"), 0 } };
}

} // namespace

Preferences::Preferences(QObject *parent)
    : QObject(parent)
{
    const QSettings &s = appSettings();
    const auto modes = QMetaEnum::fromType<TabletMode>();
    bool ok = false;
    const int mode = modes.keyToValue(s.value(kTabletMode).toString().toLatin1().constData(), &ok);
    if (ok)
        m_tabletMode = static_cast<TabletMode>(mode);
    m_stylusOnly = s.value(kStylusOnly, m_stylusOnly).toBool();
    m_barrelButtonErases = s.value(kBarrelButtonErases, m_barrelButtonErases).toBool();
    m_tapToUndo = s.value(kTapToUndo, m_tapToUndo).toBool();
    m_newNotePaper = defaultPaper();
    const QVariantMap saved = s.value(kNewNotePaper).toMap();
    for (auto it = saved.begin(); it != saved.end(); ++it)
        m_newNotePaper.insert(it.key(), it.value());
}

Preferences *Preferences::create(QQmlEngine *, QJSEngine *)
{
    Q_ASSERT(s_instance); // set in main() before QML loads
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

void Preferences::setInstance(Preferences *instance)
{
    s_instance = instance;
}

Preferences *Preferences::instance()
{
    return s_instance;
}

void Preferences::save()
{
    QSettings &s = appSettings();
    s.setValue(kTabletMode, QVariant::fromValue(m_tabletMode).toString());
    s.setValue(kStylusOnly, m_stylusOnly);
    s.setValue(kBarrelButtonErases, m_barrelButtonErases);
    s.setValue(kTapToUndo, m_tapToUndo);
    s.setValue(kNewNotePaper, m_newNotePaper);
    emit changed();
}

void Preferences::setTabletMode(TabletMode mode)
{
    if (mode == m_tabletMode)
        return;
    m_tabletMode = mode;
    save();
}

void Preferences::setStylusOnly(bool on)
{
    if (on == m_stylusOnly)
        return;
    m_stylusOnly = on;
    save();
}

void Preferences::setBarrelButtonErases(bool on)
{
    if (on == m_barrelButtonErases)
        return;
    m_barrelButtonErases = on;
    save();
}

void Preferences::setTapToUndo(bool on)
{
    if (on == m_tapToUndo)
        return;
    m_tapToUndo = on;
    save();
}

void Preferences::setNewNotePaper(const QVariantMap &paper)
{
    if (paper == m_newNotePaper)
        return;
    m_newNotePaper = paper;
    save();
}
