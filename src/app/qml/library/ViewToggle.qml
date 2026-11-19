// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

// Grid / list segmented control. The list view has no design yet, so only the
// grid is selectable for now.
Rectangle {
    id: toggle

    implicitWidth: row.implicitWidth + 6
    implicitHeight: row.implicitHeight + 6
    radius: 9
    color: Theme.segmentTrack

    Row {
        id: row

        anchors.centerIn: parent
        spacing: 2

        Repeater {
            model: [
                { icon: "layout-grid", label: qsTr("Grid view"), active: true },
                { icon: "list", label: qsTr("List view"), active: false },
            ]

            delegate: IconButton {
                required property var modelData

                width: 36
                height: 30
                radius: 7
                text: modelData.label
                iconName: modelData.icon
                iconColor: modelData.active ? Theme.text : Theme.textSecondary
                checkable: true
                checked: modelData.active
                enabled: modelData.active
                opacity: enabled ? 1 : 0.5

                background: Rectangle {
                    radius: 7
                    color: parent.checked ? Theme.surface : "transparent"
                }
            }
        }
    }
}
