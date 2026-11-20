// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Base dialog: title, content, Cancel + confirm buttons, in the app's style.
// Put the body in `content`; `canConfirm` enables the confirm button.
Dialog {
    id: dialog

    property string confirmText: qsTr("OK")
    property bool destructive: false
    property bool canConfirm: true
    default property alias content: body.data

    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(420, (parent ? parent.width : 420) - 48)
    padding: Theme.gapXl
    topPadding: Theme.gapXl

    background: Rectangle {
        radius: Theme.radiusDialog
        color: Theme.surface
        border.color: Theme.border
    }

    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, Theme.dark ? 0.5 : 0.3) }

    header: Label {
        leftPadding: Theme.gapXl
        rightPadding: Theme.gapXl
        topPadding: Theme.gapXl
        text: dialog.title
        elide: Text.ElideRight
        color: Theme.text
        font.pixelSize: Theme.fontDialogTitle
        font.weight: Font.Bold
    }

    contentItem: ColumnLayout {
        id: body

        spacing: Theme.gapM
    }

    footer: RowLayout {
        spacing: Theme.gapS

        Item { Layout.fillWidth: true }

        AppButton {
            Layout.bottomMargin: Theme.gapXl
            text: qsTr("Cancel")
            onClicked: dialog.reject()
        }

        AppButton {
            Layout.bottomMargin: Theme.gapXl
            Layout.rightMargin: Theme.gapXl
            primary: !dialog.destructive
            danger: dialog.destructive
            text: dialog.confirmText
            enabled: dialog.canConfirm
            opacity: enabled ? 1 : 0.5
            onClicked: dialog.accept()
        }
    }
}
