// SPDX-License-Identifier: GPL-3.0-or-later
#include "localization.h"

#include "appsettings.h"

#include <QCoreApplication>
#include <QJSEngine>
#include <QLocale>
#include <QLoggingCategory>
#include <QQmlEngine>

Q_LOGGING_CATEGORY(lcI18n, "tratto.i18n")

namespace {

Localization *s_instance = nullptr;

constexpr auto kSourceLanguage = "en";
const QString kLanguageKey = QStringLiteral("general/language");

} // namespace

Localization::Localization(QObject *parent)
    : QObject(parent)
    , m_language(appSettings().value(kLanguageKey).toString())
{
    apply();
}

Localization *Localization::create(QQmlEngine *, QJSEngine *)
{
    Q_ASSERT(s_instance); // set in main() before QML loads
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

Localization *Localization::instance()
{
    return s_instance;
}

void Localization::setInstance(Localization *instance)
{
    s_instance = instance;
}

void Localization::setEngine(QQmlEngine *engine)
{
    m_engine = engine;
}

void Localization::setLanguage(const QString &language)
{
    if (language == m_language)
        return;
    m_language = language;
    appSettings().setValue(kLanguageKey, language);
    apply();
}

void Localization::overrideLanguage(const QString &language)
{
    if (language == m_language)
        return;
    m_language = language;
    apply();
}

void Localization::apply()
{
    QCoreApplication::removeTranslator(&m_translator);

    const QLocale locale = m_language.isEmpty() ? QLocale::system() : QLocale(m_language);
    // Translations are embedded by qt_add_translations under :/i18n/tratto_<lang>.qm.
    // English is the source language, so it has no file and load() fails on purpose.
    if (m_translator.load(locale, QStringLiteral("tratto"), QStringLiteral("_"),
                          QStringLiteral(":/i18n"))) {
        QCoreApplication::installTranslator(&m_translator);
        m_effectiveLanguage = m_translator.language().section(QLatin1Char('_'), 0, 0);
    } else {
        m_effectiveLanguage = QString::fromLatin1(kSourceLanguage);
    }
    qCDebug(lcI18n) << "UI language:" << m_effectiveLanguage;

    QLocale::setDefault(
        m_effectiveLanguage == QLatin1String(kSourceLanguage) ? QLocale(QLocale::English) : locale);
    if (m_engine)
        m_engine->retranslate();
    emit languageChanged();
}
