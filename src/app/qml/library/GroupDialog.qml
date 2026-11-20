// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Creates a group: name and color.
AppDialog {
    id: dialog

    property color color: Library.groupColors[0]

    signal submitted(string name, color color)

    title: qsTr("New group")
    confirmText: qsTr("Create")
    canConfirm: field.text.trim().length > 0
    onOpened: {
        field.text = ""
        dialog.color = Library.groupColors[Library.groups.length % Library.groupColors.length]
        field.forceActiveFocus()
    }
    onAccepted: dialog.submitted(field.text.trim(), dialog.color)

    TextField {
        id: field

        Layout.fillWidth: true
        implicitHeight: Theme.controlHeight
        placeholderText: qsTr("Group name")
        font.pixelSize: 14
        color: Theme.text
        placeholderTextColor: Theme.textMuted
        Accessible.name: placeholderText
        onAccepted: if (dialog.canConfirm) dialog.accept()

        background: Rectangle {
            radius: Theme.radiusControl
            color: Theme.surface
            border.width: field.activeFocus ? 2 : 1
            border.color: field.activeFocus ? Theme.accentInk : Theme.borderStrong
        }
    }

    Flow {
        Layout.fillWidth: true
        spacing: Theme.gapXs

        Repeater {
            model: Library.groupColors

            delegate: AbstractButton {
                id: swatch

                required property color modelData

                implicitWidth: 40
                implicitHeight: 40
                checkable: true
                checked: Qt.colorEqual(modelData, dialog.color)
                Accessible.role: Accessible.RadioButton
                Accessible.name: modelData.toString()
                onClicked: dialog.color = modelData

                contentItem: Item {}
                background: Item {
                    Rectangle {
                        visible: swatch.checked || swatch.visualFocus
                        anchors.centerIn: parent
                        width: 32
                        height: 32
                        radius: 9
                        color: "transparent"
                        border.width: 2
                        border.color: Theme.accentInk
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: 22
                        height: 22
                        radius: 6
                        color: swatch.modelData
                    }
                }
            }
        }
    }
}
