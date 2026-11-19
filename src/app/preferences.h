// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// Input and layout preferences, exposed to QML as the `Preferences` singleton
// and persisted in appSettings(). The canvas reads them from C++ as well.
class Preferences : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(TabletMode tabletMode READ tabletMode WRITE setTabletMode NOTIFY changed FINAL)
    // Fingers never draw: one finger scrolls, two fingers zoom.
    Q_PROPERTY(bool stylusOnly READ stylusOnly WRITE setStylusOnly NOTIFY changed FINAL)
    // Holding the stylus barrel button erases (for pens without an eraser tip).
    Q_PROPERTY(bool barrelButtonErases READ barrelButtonErases WRITE setBarrelButtonErases NOTIFY
                   changed FINAL)
    // Two-finger tap undoes, three-finger tap redoes.
    Q_PROPERTY(bool tapToUndo READ tapToUndo WRITE setTapToUndo NOTIFY changed FINAL)

public:
    enum class TabletMode { Automatic, Always, Never };
    Q_ENUM(TabletMode)

    // Not default-constructible on purpose: QML must use create() and share the
    // instance owned by main(), not construct its own.
    explicit Preferences(QObject *parent);

    static Preferences *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static void setInstance(Preferences *instance);

    TabletMode tabletMode() const { return m_tabletMode; }
    void setTabletMode(TabletMode mode);
    bool stylusOnly() const { return m_stylusOnly; }
    void setStylusOnly(bool on);
    bool barrelButtonErases() const { return m_barrelButtonErases; }
    void setBarrelButtonErases(bool on);
    bool tapToUndo() const { return m_tapToUndo; }
    void setTapToUndo(bool on);

signals:
    void changed();

private:
    void save();

    TabletMode m_tabletMode = TabletMode::Automatic;
    bool m_stylusOnly = true;
    bool m_barrelButtonErases = true;
    bool m_tapToUndo = true;
};
