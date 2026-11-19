// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QSettings>

// Persistent user preferences. On Linux: ~/.config/tratto/settings.ini.
// GUI thread only.
inline QSettings &appSettings()
{
    static QSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("tratto"),
                              QStringLiteral("settings"));
    return settings;
}
