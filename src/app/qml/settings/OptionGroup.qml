// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

// Rounded card that stacks OptionRows with dividers between them.
Rectangle {
    id: group

    default property alias rows: column.data

    implicitHeight: column.implicitHeight
    radius: 14
    color: Theme.surface
    border.color: Theme.border

    ColumnLayout {
        id: column

        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0
    }
}
