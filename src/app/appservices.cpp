// SPDX-License-Identifier: GPL-3.0-or-later
#include "appservices.h"

#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcServices, "tratto.app")

AppServices::AppServices(const QString &dataDir)
    : library(dataDir, nullptr)
{
    Appearance::setInstance(&appearance);
    Localization::setInstance(&localization);
    Preferences::setInstance(&preferences);
    Library::setInstance(&library);
}

AppServices::~AppServices()
{
    // Only clear what is still ours (another AppServices may have replaced us).
    if (Library::instance() == &library)
        Library::setInstance(nullptr);
    if (Preferences::instance() == &preferences)
        Preferences::setInstance(nullptr);
    if (Localization::instance() == &localization)
        Localization::setInstance(nullptr);
    if (Appearance::instance() == &appearance)
        Appearance::setInstance(nullptr);
}

void AppServices::loadUiFont()
{
    const int id = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/InterVariable.ttf"));
    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    if (families.isEmpty()) {
        qCWarning(lcServices) << "Could not load the bundled UI font, using the system font";
        return;
    }
    QFont font(families.first());
    font.setPixelSize(14);
    QGuiApplication::setFont(font);
}
