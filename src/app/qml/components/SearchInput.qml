// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

// 44 px search field with a leading search icon.
TextField {
    id: control

    implicitHeight: Theme.controlHeight
    leftPadding: 14 + 18 + Theme.gapS
    rightPadding: 14
    font.pixelSize: 14
    color: Theme.text
    placeholderTextColor: Theme.textMuted
    selectByMouse: true
    Accessible.name: placeholderText

    background: Rectangle {
        radius: Theme.radiusControl
        color: Theme.surface
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? Theme.accentInk : Theme.border

        Icon {
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            name: "search"
            color: Theme.textMuted
        }
    }
}
