// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Editor top bar: back, title and group, save status, history, view toggles, export.
Rectangle {
    id: bar

    required property NoteSession session
    property bool pagesPanelOpen: true

    signal back()
    signal togglePagesPanel()

    readonly property var group: {
        Library.groups
        return session.groupId !== "" ? Library.group(session.groupId) : ({})
    }

    implicitHeight: Theme.topBarHeight
    color: Theme.surface

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 6

        BackButton {
            objectName: "editorBack"
            text: qsTr("Library")
            Accessible.name: qsTr("Back to the library")
            onClicked: bar.back()
        }

        Divider { length: 24; Layout.leftMargin: 0; Layout.rightMargin: 0 }

        RowLayout {
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.fillWidth: true
            Layout.maximumWidth: implicitWidth
            spacing: 10

            // Title: click to rename.
            TextField {
                id: titleField

                Layout.fillWidth: true
                Layout.minimumWidth: 80
                Layout.preferredWidth: Math.max(80, contentWidth + leftPadding + rightPadding + 4)
                implicitHeight: 36
                leftPadding: 6
                rightPadding: 6
                text: bar.session.title
                placeholderText: qsTr("Untitled note")
                color: Theme.text
                placeholderTextColor: Theme.textMuted
                font.pixelSize: Theme.fontSection
                font.weight: Font.DemiBold
                selectByMouse: true
                Accessible.name: qsTr("Note title")
                onEditingFinished: {
                    bar.session.title = text
                    focus = false
                }
                Keys.onEscapePressed: {
                    text = bar.session.title
                    focus = false
                }

                background: Rectangle {
                    radius: 8
                    color: titleField.activeFocus ? Theme.surface
                         : titleField.hovered ? Theme.hover : "transparent"
                    border.width: titleField.activeFocus ? 2 : 0
                    border.color: Theme.accentInk
                }
            }

            // Group chip
            Rectangle {
                visible: bar.group.id !== undefined
                implicitWidth: chip.implicitWidth + 16
                implicitHeight: 24
                radius: 6
                color: Theme.background

                Row {
                    id: chip

                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 8
                        height: 8
                        radius: 2
                        color: bar.group.color ?? "transparent"
                    }

                    Label {
                        text: bar.group.name ?? ""
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontLabel
                        font.weight: Font.Medium
                    }
                }
            }
        }

        // Save status
        RowLayout {
            spacing: 6
            Accessible.role: Accessible.StatusBar

            Icon {
                name: "check"
                size: 16
                color: Theme.textMuted
                opacity: bar.session.saving ? 0 : 1
            }

            Label {
                text: bar.session.saving ? qsTr("Saving…") : qsTr("Saved automatically")
                color: Theme.textMuted
                font.pixelSize: 13
            }
        }

        Item { Layout.fillWidth: true }

        IconButton {
            size: Theme.controlHeight
            radius: Theme.radiusControl
            iconSize: 20
            iconStrokeWidth: 1.8
            iconName: "undo"
            text: qsTr("Undo (Ctrl+Z)")
            enabled: bar.session.canUndo
            onClicked: bar.session.undo()
        }

        IconButton {
            size: Theme.controlHeight
            radius: Theme.radiusControl
            iconSize: 20
            iconStrokeWidth: 1.8
            iconName: "redo"
            text: qsTr("Redo (Ctrl+Shift+Z)")
            enabled: bar.session.canRedo
            onClicked: bar.session.redo()
        }

        Divider { length: 24; Layout.leftMargin: 0; Layout.rightMargin: 0 }

        IconButton {
            size: Theme.controlHeight
            radius: Theme.radiusControl
            iconSize: 20
            iconStrokeWidth: 1.8
            iconName: "tablet"
            text: qsTr("Tablet mode")
            enabled: false // M5
        }

        IconButton {
            size: Theme.controlHeight
            radius: Theme.radiusControl
            iconSize: 20
            iconStrokeWidth: 1.8
            iconName: "panel-right"
            text: qsTr("Pages")
            active: bar.pagesPanelOpen
            Accessible.checkable: true
            Accessible.checked: bar.pagesPanelOpen
            onClicked: bar.togglePagesPanel()
        }

        AppButton {
            Layout.leftMargin: 4
            implicitHeight: 40
            leftPadding: 14
            rightPadding: 14
            text: qsTr("Export")
            iconName: "upload"
            enabled: false // M4
            opacity: enabled ? 1 : 0.5
        }

        IconButton {
            size: Theme.controlHeight
            radius: Theme.radiusControl
            iconSize: 20
            iconName: "ellipsis"
            iconFilled: true
            text: qsTr("More options")
            enabled: false
        }
    }
}
