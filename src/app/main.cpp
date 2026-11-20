// SPDX-License-Identifier: GPL-3.0-or-later
#include "appearance.h"
#include "localization.h"
#include "preferences.h"

#include <QCommandLineParser>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QTimer>

Q_LOGGING_CATEGORY(lcApp, "tratto.app")

namespace {

// Time for the first frames (fonts, thumbnails) to settle before a screenshot.
constexpr int kScreenshotDelayMs = 1500;

// Makes the bundled Inter the default UI font; falls back to the system font.
void loadUiFont()
{
    const int id = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/InterVariable.ttf"));
    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    if (families.isEmpty()) {
        qCWarning(lcApp) << "Could not load the bundled UI font, using the system font";
        return;
    }
    QFont font(families.first());
    font.setPixelSize(14);
    QGuiApplication::setFont(font);
}

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
        QStringLiteral("Open <page> at startup: library, settings or editor (development)."),
        QStringLiteral("page"), QStringLiteral("library"));
    parser.addOptions({ colorSchemeOption, languageOption, screenshotOption, pageOption });
    parser.process(app);

    loadUiFont();
    // Basic is the most themeable Qt Quick Controls style; the look comes from Theme.qml.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    Appearance appearance(nullptr);
    Appearance::setInstance(&appearance);
    const QString scheme = parser.value(colorSchemeOption);
    if (scheme == QLatin1String("light"))
        appearance.overrideMode(Appearance::Mode::Light);
    else if (scheme == QLatin1String("dark"))
        appearance.overrideMode(Appearance::Mode::Dark);

    Localization localization(nullptr);
    Localization::setInstance(&localization);
    if (parser.isSet(languageOption))
        localization.overrideLanguage(parser.value(languageOption));

    Preferences preferences(nullptr);
    Preferences::setInstance(&preferences);

    QQmlApplicationEngine engine;
    localization.setEngine(&engine);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.setInitialProperties({ { QStringLiteral("startPage"), parser.value(pageOption) } });
    engine.loadFromModule("Tratto", "Main");

    if (parser.isSet(screenshotOption))
        scheduleScreenshot(engine, parser.value(screenshotOption));

    return QGuiApplication::exec();
}
