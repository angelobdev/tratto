// SPDX-License-Identifier: GPL-3.0-or-later
#include "appservices.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QTimer>
#include <QtQml/QQmlExtensionPlugin>

// The QML module lives in the static tratto_ui library.
Q_IMPORT_QML_PLUGIN(TrattoPlugin)

Q_LOGGING_CATEGORY(lcApp, "tratto.app")

namespace {

// Time for the first frames (fonts, thumbnails) to settle before a screenshot.
constexpr int kScreenshotDelayMs = 1500;

// Saves the first window to `path` once it has rendered, then quits.
// Used for visual checks during development and in CI.
void scheduleScreenshot(QQmlApplicationEngine &engine, const QString &path)
{
    QTimer::singleShot(kScreenshotDelayMs, &engine, [&engine, path] {
        const auto roots = engine.rootObjects();
        auto *window = roots.isEmpty() ? nullptr : qobject_cast<QQuickWindow *>(roots.first());
        int status = 1;
        if (window && window->grabWindow().save(path))
            status = 0;
        else
            qCWarning(lcApp) << "Could not save screenshot to" << path;
        QCoreApplication::exit(status);
    });
}

} // namespace

int main(int argc, char *argv[])
{
    // 4x multisampling: smooth edges for the live stroke, drawn as GPU triangles.
    // Must be set before the application object exists.
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("tratto"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Tratto"));
    QGuiApplication::setApplicationVersion(QStringLiteral(TRATTO_VERSION));
    // Also the Wayland app_id, so compositors and launchers can match the .desktop file.
    QGuiApplication::setDesktopFileName(QStringLiteral(TRATTO_APP_ID));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption colorSchemeOption(
        QStringLiteral("color-scheme"), QStringLiteral("Force the color scheme: light or dark."),
        QStringLiteral("scheme"));
    const QCommandLineOption languageOption(QStringLiteral("language"),
                                            QStringLiteral("Force the UI language, e.g. en or it."),
                                            QStringLiteral("code"));
    const QCommandLineOption screenshotOption(
        QStringLiteral("screenshot"),
        QStringLiteral("Save a screenshot of the window to <file> and quit (development)."),
        QStringLiteral("file"));
    const QCommandLineOption pageOption(
        QStringLiteral("page"),
        QStringLiteral(
            "Open <page> at startup: library, settings, editor or new-note (development)."),
        QStringLiteral("page"), QStringLiteral("library"));
    const QCommandLineOption dataDirOption(
        QStringLiteral("data-dir"),
        QStringLiteral("Keep notes in <dir> instead of the user data folder (development)."),
        QStringLiteral("dir"));
    parser.addOptions(
        { colorSchemeOption, languageOption, screenshotOption, pageOption, dataDirOption });
    parser.process(app);

    AppServices::loadUiFont();
    // Basic is the most themeable Qt Quick Controls style; the look comes from Theme.qml.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // Before the engine, so it is destroyed after it: editor sessions flush their
    // last changes and the I/O thread finishes them before exit.
    AppServices services(parser.isSet(dataDirOption)
                             ? parser.value(dataDirOption)
                             : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    const QString scheme = parser.value(colorSchemeOption);
    if (scheme == QLatin1String("light"))
        services.appearance.overrideMode(Appearance::Mode::Light);
    else if (scheme == QLatin1String("dark"))
        services.appearance.overrideMode(Appearance::Mode::Dark);
    if (parser.isSet(languageOption))
        services.localization.overrideLanguage(parser.value(languageOption));

    QQmlApplicationEngine engine;
    services.localization.setEngine(&engine);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.setInitialProperties({ { QStringLiteral("startPage"), parser.value(pageOption) } });
    engine.loadFromModule("Tratto", "Main");

    if (parser.isSet(screenshotOption))
        scheduleScreenshot(engine, parser.value(screenshotOption));

    return QGuiApplication::exec();
}
