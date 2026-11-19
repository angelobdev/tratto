// SPDX-License-Identifier: GPL-3.0-or-later
#include "appearance.h"
#include "localization.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("tratto"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Tratto"));
    QGuiApplication::setApplicationVersion(QStringLiteral(TRATTO_VERSION));
    // Also the Wayland app_id, so compositors and launchers can match the .desktop file.
    QGuiApplication::setDesktopFileName(QStringLiteral(TRATTO_APP_ID));

    // Basic is the most themeable Qt Quick Controls style; the look comes from Theme.qml.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    Appearance appearance(nullptr);
    Appearance::setInstance(&appearance);
    Localization localization(nullptr);
    Localization::setInstance(&localization);

    QQmlApplicationEngine engine;
    localization.setEngine(&engine);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("Tratto", "Main");

    return QGuiApplication::exec();
}
