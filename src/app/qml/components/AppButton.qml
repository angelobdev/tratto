// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 44 px button: `primary` is filled with the accent, otherwise a bordered surface.
Button {
    id: control

    property bool primary: false
    property string iconName

    implicitHeight: Theme.controlHeight
    leftPadding: primary ? 18 : 16
    rightPadding: leftPadding
    font.pixelSize: 14
    font.weight: Font.DemiBold

    readonly property color foreground: primary ? Theme.accentForeground : Theme.text

    contentItem: RowLayout {
        spacing: Theme.gapS

        Icon {
            visible: control.iconName !== ""
            name: control.iconName
            color: control.foreground
            strokeWidth: control.primary ? 2 : 1.8
        }

        Label {
            text: control.text
            font: control.font
            color: control.foreground
        }
    }

    background: Rectangle {
        radius: Theme.radiusControl
        color: control.primary
               ? (control.down ? Qt.darker(Theme.accent, 1.15) : control.hovered ? Qt.darker(Theme.accent, 1.07) : Theme.accent)
               : (control.down ? Theme.surfaceAlt : control.hovered ? Theme.hover : Theme.surface)
        border.width: control.primary ? 0 : 1
        border.color: Theme.borderStrong

        FocusRing { visible: control.visualFocus }
    }
}
