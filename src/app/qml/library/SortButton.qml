// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// "Recently modified ⌄": picks the note sort order.
AbstractButton {
    id: control

    property string sortBy: "modified" // "modified" | "title"

    implicitHeight: 36
    leftPadding: 12
    rightPadding: 12
    hoverEnabled: true
    text: sortBy === "title" ? qsTr("Title") : qsTr("Recently modified")
    Accessible.name: qsTr("Sort by: %1").arg(text)

    onClicked: menu.open()

    contentItem: RowLayout {
        spacing: 6

        Label {
            text: control.text
            color: Theme.textSecondary
            font.pixelSize: 13
            font.weight: Font.Medium
        }

        Icon {
            name: "chevron-down"
            size: 16
            strokeWidth: 2
        }
    }

    background: Rectangle {
        radius: 8
        color: control.down ? Theme.pressed : control.hovered ? Theme.hover : "transparent"

        FocusRing { visible: control.visualFocus }
    }

    Menu {
        id: menu

        y: control.height

        MenuItem {
            text: qsTr("Recently modified")
            checkable: true
            checked: control.sortBy === "modified"
            onTriggered: control.sortBy = "modified"
        }

        MenuItem {
            text: qsTr("Title")
            checkable: true
            checked: control.sortBy === "title"
            onTriggered: control.sortBy = "title"
        }
    }
}
