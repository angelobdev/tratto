// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Asks for one line of text (renaming a note or a group).
AppDialog {
    id: dialog

    property alias text: field.text
    property alias placeholderText: field.placeholderText

    signal submitted(string text)

    canConfirm: field.text.trim().length > 0
    onOpened: {
        field.forceActiveFocus()
        field.selectAll()
    }
    onAccepted: dialog.submitted(field.text.trim())

    TextField {
        id: field

        Layout.fillWidth: true
        implicitHeight: Theme.controlHeight
        font.pixelSize: 14
        color: Theme.text
        placeholderTextColor: Theme.textMuted
        selectByMouse: true
        Accessible.name: dialog.title
        onAccepted: if (dialog.canConfirm) dialog.accept()

        background: Rectangle {
            radius: Theme.radiusControl
            color: Theme.surface
            border.width: field.activeFocus ? 2 : 1
            border.color: field.activeFocus ? Theme.accentInk : Theme.borderStrong
        }
    }
}
