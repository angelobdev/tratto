// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

// Compact checkbox with a label, for toolbars.
CheckBox {
    id: control

    implicitHeight: Theme.controlHeight
    leftPadding: 8
    rightPadding: 8
    spacing: 8
    hoverEnabled: true

    indicator: Rectangle {
        x: control.leftPadding
        anchors.verticalCenter: parent.verticalCenter
        width: 16
        height: 16
        radius: 4
        color: control.checked ? Theme.accent : Theme.surface
        border.width: control.checked ? 0 : 1.5
        border.color: Theme.textMuted

        Icon {
            visible: control.checked
            anchors.centerIn: parent
            name: "check"
            size: 12
            strokeWidth: 3
            color: Theme.accentForeground
        }

        FocusRing { visible: control.visualFocus }
    }

    contentItem: Label {
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        verticalAlignment: Text.AlignVCenter
        color: Theme.textSecondary
        font.pixelSize: 13
        font.weight: Font.Medium
    }
}
