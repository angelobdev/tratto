// SPDX-License-Identifier: GPL-3.0-or-later
#include "appearance.h"

#include "appsettings.h"
#include "tratto/core/color.h"

#include <QGuiApplication>
#include <QJSEngine>
#include <QMetaEnum>
#include <QStyleHints>

using tratto::core::Rgba;

namespace {

Appearance *s_instance = nullptr;

const QString kModeKey = QStringLiteral("appearance/mode");
const QString kAccentKey = QStringLiteral("appearance/accent");

Rgba toRgba(const QColor &color)
{
    const QColor rgb = color.toRgb();
    return { static_cast<std::uint8_t>(rgb.red()), static_cast<std::uint8_t>(rgb.green()),
             static_cast<std::uint8_t>(rgb.blue()), static_cast<std::uint8_t>(rgb.alpha()) };
}

QColor toQColor(Rgba color)
{
    return QColor(color.r, color.g, color.b, color.a);
}

} // namespace

Appearance::Appearance(QObject *parent)
    : QObject(parent)
    , m_accent(toQColor(tratto::core::kDefaultAccent))
{
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
            &Appearance::updateDark);

    const QSettings &settings = appSettings();
    const QColor accent(settings.value(kAccentKey).toString());
    if (accent.isValid())
        m_accent = accent;
    bool ok = false;
    const int mode = QMetaEnum::fromType<Mode>().keyToValue(
        settings.value(kModeKey).toString().toLatin1().constData(), &ok);
    if (ok)
        applyMode(static_cast<Mode>(mode));
    updateDark();
}

Appearance *Appearance::create(QQmlEngine *, QJSEngine *)
{
    Q_ASSERT(s_instance); // set in main() before QML loads
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

void Appearance::setInstance(Appearance *instance)
{
    s_instance = instance;
}

void Appearance::setMode(Mode mode)
{
    if (mode == m_mode)
        return;
    applyMode(mode);
    appSettings().setValue(kModeKey, QVariant::fromValue(mode).toString());
}

void Appearance::overrideMode(Mode mode)
{
    if (mode != m_mode)
        applyMode(mode);
}

void Appearance::applyMode(Mode mode)
{
    m_mode = mode;

    // Telling Qt (instead of only our QML) also makes window decorations and
    // native dialogs follow the chosen scheme.
    QStyleHints *hints = QGuiApplication::styleHints();
    switch (mode) {
    case Mode::System: hints->unsetColorScheme(); break;
    case Mode::Light: hints->setColorScheme(Qt::ColorScheme::Light); break;
    case Mode::Dark: hints->setColorScheme(Qt::ColorScheme::Dark); break;
    }

    emit modeChanged();
    updateDark();
}

void Appearance::updateDark()
{
    const bool dark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    if (dark == m_dark)
        return;
    m_dark = dark;
    emit darkChanged();
    emit paletteChanged();
}

void Appearance::setAccent(const QColor &accent)
{
    if (!accent.isValid())
        return;
    QColor opaque = accent.toRgb();
    opaque.setAlpha(255);
    if (opaque == m_accent)
        return;
    m_accent = opaque;
    appSettings().setValue(kAccentKey, opaque.name());
    emit paletteChanged();
}

QColor Appearance::accentForeground() const
{
    return toQColor(tratto::core::accentPalette(toRgba(m_accent), m_dark).onAccent);
}

QColor Appearance::accentSoft() const
{
    return toQColor(tratto::core::accentPalette(toRgba(m_accent), m_dark).soft);
}

QColor Appearance::accentInk() const
{
    return toQColor(tratto::core::accentPalette(toRgba(m_accent), m_dark).ink);
}

QList<QColor> Appearance::accentPresets() const
{
    QList<QColor> presets;
    presets.reserve(static_cast<qsizetype>(tratto::core::kAccentPresets.size()));
    for (const Rgba preset : tratto::core::kAccentPresets)
        presets.append(toQColor(preset));
    return presets;
}

QColor Appearance::mixWithWhite(const QColor &color, qreal amount) const
{
    return toQColor(tratto::core::mixWithWhite(toRgba(color), amount));
}
