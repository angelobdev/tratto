// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

// Context menu of a note card (right click or press and hold).
// Notes in the trash get Restore / Delete forever instead.
Item {
    id: root

    property var note: ({})

    signal openRequested()
    signal renameRequested()
    signal deleteForeverRequested()

    function popup() {
        (root.note.inTrash ? trashMenu : menu).popup()
    }

    Menu {
        id: menu

        objectName: "noteMenu"
        MenuItem { text: qsTr("Open"); onTriggered: root.openRequested() }
        MenuItem { text: qsTr("Rename…"); onTriggered: root.renameRequested() }

        Menu {
            id: moveMenu

            title: qsTr("Move to")

            MenuItem {
                text: qsTr("Standalone notes")
                checkable: true
                checked: root.note.group === ""
                onTriggered: Library.moveNote(root.note.id, "")
            }

            Instantiator {
                model: Library.groups

                delegate: MenuItem {
                    required property var modelData

                    text: modelData.name
                    checkable: true
                    checked: root.note.group === modelData.id
                    onTriggered: Library.moveNote(root.note.id, modelData.id)
                }
                onObjectAdded: (index, object) => moveMenu.insertItem(index + 1, object)
                onObjectRemoved: (index, object) => moveMenu.removeItem(object)
            }
        }

        MenuSeparator {}

        MenuItem {
            text: qsTr("Move to Trash")
            onTriggered: Library.trashNote(root.note.id)
        }
    }

    Menu {
        id: trashMenu

        objectName: "trashMenu"
        MenuItem { text: qsTr("Restore"); onTriggered: Library.restoreNote(root.note.id) }
        MenuItem { text: qsTr("Delete forever…"); onTriggered: root.deleteForeverRequested() }
    }
}
