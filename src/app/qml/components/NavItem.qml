// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Sidebar entry: icon (or group color square), label and optional count.
AbstractButton {
    id: control

    property string iconName
    property color groupColor: "transparent" // set for group entries instead of an icon
    property int count: -1                   // -1 hides the count
    property bool current: false

    implicitHeight: 40
    leftPadding: 10
    rightPadding: 10
    hoverEnabled: true
    Accessible.role: Accessible.PageTab
    Accessible.selected: current

    readonly property color foreground: current ? Theme.accentInk : Theme.textSecondary

    contentItem: RowLayout {
        spacing: 10

        Icon {
            visible: control.iconName !== ""
            name: control.iconName
            color: control.foreground
        }

        Rectangle {
            visible: control.iconName === ""
            Layout.leftMargin: 4
            Layout.rightMargin: 4
            implicitWidth: 10
            implicitHeight: 10
            radius: 3
            color: control.groupColor
        }

        Label {
            Layout.fillWidth: true
            text: control.text
            elide: Text.ElideRight
            color: control.foreground
            font.pixelSize: 14
            font.weight: control.current ? Font.DemiBold : Font.Medium
        }

        Label {
            visible: control.count >= 0
            text: control.count
            color: control.current ? Theme.accentInk : Theme.textMuted
            font.pixelSize: 12
            font.weight: control.current ? Font.DemiBold : Font.Normal
        }
    }

    background: Rectangle {
        radius: 8
        color: control.current ? Theme.accentSoft
             : control.down ? Theme.pressed
             : control.hovered ? Theme.hover : "transparent"

        FocusRing { visible: control.visualFocus }
    }
}
