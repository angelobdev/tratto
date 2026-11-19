// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

// 32 px color dot in a 48 px touch target, with a ring when selected.
AbstractButton {
    id: swatch

    property color color

    implicitWidth: 48
    implicitHeight: 48
    checkable: true
    hoverEnabled: true
    Accessible.role: Accessible.RadioButton
    Accessible.name: text

    ToolTip.visible: hovered
    ToolTip.text: text
    ToolTip.delay: 600

    contentItem: Item {}

    background: Item {
        // Selection ring: page background gap, then the accent (DESIGN.md).
        Rectangle {
            visible: swatch.checked || swatch.visualFocus
            anchors.centerIn: parent
            width: 42
            height: 42
            radius: 21
            color: "transparent"
            border.width: 2
            border.color: Theme.accentInk
        }

        Rectangle {
            anchors.centerIn: parent
            width: swatch.hovered && !swatch.checked ? 34 : 32
            height: width
            radius: width / 2
            color: swatch.color

            Behavior on width { NumberAnimation { duration: 80 } }
        }
    }
}
