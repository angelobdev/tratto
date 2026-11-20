// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "appearance.h"
#include "library.h"
#include "localization.h"
#include "preferences.h"

#include <QString>

// The app-wide objects behind the QML singletons, created in the right order and
// registered as the instances QML uses. main() and the UI tests both use this.
// Create it after the QGuiApplication and before the QQmlEngine, and destroy it
// after the engine: the Library then finishes queued saves before exiting.
struct AppServices
{
    explicit AppServices(const QString &dataDir);
    ~AppServices();

    // Makes the bundled Inter the default UI font; falls back to the system font.
    static void loadUiFont();

    Appearance appearance { nullptr };
    Localization localization { nullptr };
    Preferences preferences { nullptr };
    Library library;
};
