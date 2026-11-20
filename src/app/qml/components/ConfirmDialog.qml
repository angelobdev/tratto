// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Asks to confirm an action, usually a destructive one.
AppDialog {
    id: dialog

    property string message

    destructive: true

    Label {
        Layout.fillWidth: true
        text: dialog.message
        wrapMode: Text.Wrap
        color: Theme.textSecondary
        font.pixelSize: 14
    }
}
