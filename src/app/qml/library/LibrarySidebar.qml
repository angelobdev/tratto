// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Library navigation: sections, groups, trash and settings.
// `section` is "all", "recent", "standalone", "trash" or "group:<id>".
Rectangle {
    id: sidebar

    property string section: "all"

    signal settingsRequested()

    implicitWidth: Theme.sidebarWidth
    color: Theme.sidebar

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.border
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 20
        anchors.bottomMargin: 20
        anchors.leftMargin: 14
        anchors.rightMargin: 15 // 14 + the 1 px border
        spacing: Theme.gapXs

        // Brand
        RowLayout {
            Layout.leftMargin: 10
            Layout.topMargin: 4
            Layout.bottomMargin: 18
            spacing: 10

            Rectangle {
                implicitWidth: 32
                implicitHeight: 32
                radius: 9
                color: Theme.accent

                Icon {
                    anchors.centerIn: parent
                    name: "pen-line"
                    color: Theme.accentForeground
                    strokeWidth: 2
                }
            }

            Label {
                text: "Tratto"
                color: Theme.text
                font.pixelSize: 17
                font.weight: Font.Bold
                font.letterSpacing: -0.17
            }
        }

        NavItem {
            Layout.fillWidth: true
            text: qsTr("All notes")
            iconName: "notebook-text"
            count: FakeLibrary.notes.length
            current: sidebar.section === "all"
            onClicked: sidebar.section = "all"
        }

        NavItem {
            Layout.fillWidth: true
            text: qsTr("Recent")
            iconName: "clock"
            current: sidebar.section === "recent"
            onClicked: sidebar.section = "recent"
        }

        NavItem {
            Layout.fillWidth: true
            text: qsTr("Standalone notes")
            iconName: "file"
            count: FakeLibrary.notesInGroup("").length
            current: sidebar.section === "standalone"
            onClicked: sidebar.section = "standalone"
        }

        // Groups header
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 22
            Layout.bottomMargin: 6
            Layout.leftMargin: 10
            Layout.rightMargin: 10

            Label {
                Layout.fillWidth: true
                text: qsTr("Groups").toUpperCase()
                color: Theme.textMuted
                font.pixelSize: Theme.fontLabel
                font.weight: Font.DemiBold
                font.letterSpacing: Theme.fontLabel * 0.06
            }

            IconButton {
                text: qsTr("New group")
                iconName: "plus"
                enabled: false // groups are created in M2
                opacity: enabled ? 1 : 0.4
            }
        }

        Repeater {
            model: FakeLibrary.groups

            delegate: NavItem {
                required property var modelData

                Layout.fillWidth: true
                text: modelData.name
                groupColor: modelData.color
                count: FakeLibrary.notesInGroup(modelData.id).length
                current: sidebar.section === "group:" + modelData.id
                onClicked: sidebar.section = "group:" + modelData.id
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.minimumHeight: 24
        }

        NavItem {
            Layout.fillWidth: true
            text: qsTr("Trash")
            iconName: "trash"
            current: sidebar.section === "trash"
            onClicked: sidebar.section = "trash"
        }

        NavItem {
            Layout.fillWidth: true
            text: qsTr("Settings")
            iconName: "sliders-vertical"
            onClicked: sidebar.settingsRequested()
        }
    }
}
