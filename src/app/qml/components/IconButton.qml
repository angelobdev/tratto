// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

// Icon-only button. Always set `text`: it is the accessible name and the tooltip.
AbstractButton {
    id: control

    property string iconName
    property real iconSize: 16
    property real size: 28
    property int radius: 6
    property real iconStrokeWidth: 2
    property bool iconFilled: false
    property color iconColor: active ? Theme.accentInk : Theme.textSecondary
    // Selected state (current tool, open panel): accent-tinted background.
    property bool active: false

    opacity: enabled ? 1 : 0.4
    implicitWidth: size
    implicitHeight: size
    hoverEnabled: true
    display: AbstractButton.IconOnly
    Accessible.name: text

    ToolTip.visible: hovered && text !== ""
    ToolTip.text: text
    ToolTip.delay: 600

    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: control.iconName
            size: control.iconSize
            strokeWidth: control.iconStrokeWidth
            filled: control.iconFilled
            color: control.iconColor
        }
    }

    background: Rectangle {
        radius: control.radius
        color: control.active ? Theme.accentSoft
             : control.down ? Theme.pressed
             : control.hovered ? Theme.hover : "transparent"

        FocusRing { visible: control.visualFocus }
    }
}
