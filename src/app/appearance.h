// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QColor>
#include <QList>
#include <QObject>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// Light/dark mode and accent color, exposed to QML as the `Appearance` singleton.
// The static design tokens live in Theme.qml; this class owns the parts that
// change at runtime and the accent math from tratto_core.
class Appearance : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged FINAL)
    Q_PROPERTY(bool dark READ dark NOTIFY darkChanged FINAL)
    Q_PROPERTY(QColor accent READ accent WRITE setAccent NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor onAccent READ onAccent NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor accentSoft READ accentSoft NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor accentInk READ accentInk NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QList<QColor> accentPresets READ accentPresets CONSTANT FINAL)

public:
    enum class Mode { System, Light, Dark };
    Q_ENUM(Mode)

    // Not default-constructible on purpose: QML must use create() and share the
    // instance owned by main(), not construct its own.
    explicit Appearance(QObject *parent);

    // QML singleton factory: hands QML the instance owned by main().
    static Appearance *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static void setInstance(Appearance *instance);

    Mode mode() const { return m_mode; }
    void setMode(Mode mode);
    bool dark() const { return m_dark; }

    QColor accent() const { return m_accent; }
    void setAccent(const QColor &accent);
    QColor onAccent() const;
    QColor accentSoft() const;
    QColor accentInk() const;
    QList<QColor> accentPresets() const;

    // Moves `color` towards white by `amount` (0..1), e.g. for group tints.
    Q_INVOKABLE QColor mixWithWhite(const QColor &color, qreal amount) const;

signals:
    void modeChanged();
    void darkChanged();
    void paletteChanged();

private:
    void updateDark();

    Mode m_mode = Mode::System;
    bool m_dark = false;
    QColor m_accent;
};
