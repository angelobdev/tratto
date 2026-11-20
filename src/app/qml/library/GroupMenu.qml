// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

// Context menu of a group (sidebar entry or group card).
Menu {
    id: menu

    objectName: "groupMenu"
    property var group: ({})

    signal renameRequested()
    signal deleteRequested()

    MenuItem { text: qsTr("Rename…"); onTriggered: menu.renameRequested() }
    MenuItem { text: qsTr("Delete group…"); onTriggered: menu.deleteRequested() }
}
