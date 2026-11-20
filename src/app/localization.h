// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QString>
#include <QTranslator>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// UI language, exposed to QML as the `Localization` singleton.
// Switching language reloads the translator and retranslates every qsTr() live.
class Localization : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // "" follows the system language; otherwise a language code such as "en" or "it".
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged FINAL)
    Q_PROPERTY(QString effectiveLanguage READ effectiveLanguage NOTIFY languageChanged FINAL)

public:
    // Not default-constructible on purpose: QML must use create() and share the
    // instance owned by main(), not construct its own.
    explicit Localization(QObject *parent);

    static Localization *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static void setInstance(Localization *instance);
    static Localization *instance();

    // Must be called once the QML engine exists, so language changes can retranslate it.
    void setEngine(QQmlEngine *engine);

    QString language() const { return m_language; }
    void setLanguage(const QString &language); // saved to the settings
    void overrideLanguage(const QString &language); // this session only (command line)
    QString effectiveLanguage() const { return m_effectiveLanguage; }

signals:
    void languageChanged();

private:
    void apply();

    QString m_language;
    QString m_effectiveLanguage;
    QTranslator m_translator;
    QQmlEngine *m_engine = nullptr;
};
