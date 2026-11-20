// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Library: sidebar navigation, header with search and actions, groups and notes.
Page {
    id: page

    signal openNote(string noteId)
    signal openSettings()

    background: Rectangle { color: Theme.background }

    // "recent" = modified in the last week
    readonly property int recentDays: 7
    property string sortBy: "modified" // "modified" | "title"

    readonly property string section: sidebar.section
    readonly property string groupId: section.startsWith("group:") ? section.slice(6) : ""
    readonly property string query: search.text.trim().toLowerCase()

    readonly property var visibleNotes: {
        let notes = Library.notes.filter(n => n.inTrash === (section === "trash"))
        if (section === "recent")
            notes = notes.filter(n => Format.daysAgo(n.modified) < recentDays)
        else if (section === "standalone")
            notes = notes.filter(n => n.group === "")
        else if (groupId !== "")
            notes = notes.filter(n => n.group === groupId)
        if (query !== "")
            notes = notes.filter(n => n.title.toLowerCase().includes(query))
        if (sortBy === "title")
            notes = notes.slice().sort((a, b) => a.title.localeCompare(b.title))
        return notes
    }

    readonly property string heading: {
        switch (section) {
        case "recent": return qsTr("Recent")
        case "standalone": return qsTr("Standalone notes")
        case "trash": return qsTr("Trash")
        case "all": return qsTr("All notes")
        default: return Library.groups.find(g => g.id === groupId)?.name ?? ""
        }
    }

    readonly property string subtitle: section === "all"
        ? Format.noteCount(visibleNotes.length) + " · " + Format.groupCount(Library.groups.length)
        : Format.noteCount(visibleNotes.length)

    RowLayout {
        anchors.fill: parent
        spacing: 0

        LibrarySidebar {
            id: sidebar

            Layout.fillHeight: true
            onSettingsRequested: page.openSettings()
            onNewGroupRequested: groupDialog.open()
            onGroupMenuRequested: group => page.showGroupMenu(group)
        }

        ScrollView {
            id: scroll

            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth

            ColumnLayout {
                width: scroll.availableWidth
                spacing: 28

                // Header: wraps the actions below the title when the window is narrow.
                Flow {
                    Layout.fillWidth: true
                    Layout.topMargin: 28
                    Layout.leftMargin: 36
                    Layout.rightMargin: 36
                    spacing: Theme.gapL

                    ColumnLayout {
                        // Shares the row with the actions while it keeps at least 200 px, else takes
                        // the whole row and the actions wrap below.
                        readonly property real available: parent.width - actions.implicitWidth - parent.spacing
                        width: available >= 200 ? available : parent.width
                        height: actions.implicitHeight
                        spacing: Theme.gapXs

                        Label {
                            text: page.heading
                            color: Theme.text
                            font.pixelSize: Theme.fontTitle
                            font.weight: Font.Bold
                            font.letterSpacing: -0.52
                        }

                        Label {
                            text: page.subtitle
                            color: Theme.textMuted
                            font.pixelSize: 13
                        }
                    }

                    RowLayout {
                        id: actions

                        // When the actions wrap below the title, the search field shrinks to fit.
                        width: Math.min(implicitWidth, parent.width)
                        spacing: Theme.gapL

                        SearchInput {
                            id: search

                            Layout.fillWidth: true
                            Layout.preferredWidth: 320
                            Layout.maximumWidth: 320
                            Layout.minimumWidth: 140
                            placeholderText: qsTr("Search notes")
                        }

                        AppButton {
                            text: qsTr("Import")
                            iconName: "download"
                            enabled: false // PDF/image import arrives in M4
                            opacity: enabled ? 1 : 0.5
                        }

                        AppButton {
                            primary: true
                            text: qsTr("New note")
                            iconName: "plus"
                            onClicked: page.openNewNote()
                        }
                    }
                }

                // Groups (only in "All notes", when not searching)
                ColumnLayout {
                    visible: page.section === "all" && page.query === "" && Library.groups.length > 0
                    Layout.fillWidth: true
                    Layout.leftMargin: 36
                    Layout.rightMargin: 36
                    spacing: 14

                    Label {
                        text: qsTr("Groups")
                        color: Theme.text
                        font.pixelSize: Theme.fontSection
                        font.weight: Font.DemiBold
                    }

                    ResponsiveGrid {
                        Layout.fillWidth: true
                        minCellWidth: 220
                        columnSpacing: Theme.gapL
                        rowSpacing: Theme.gapL

                        Repeater {
                            model: Library.groups

                            delegate: GroupCard {
                                required property var modelData
                                readonly property var members: Library.notes.filter(n => !n.inTrash && n.group === modelData.id)

                                width: parent.cellWidth
                                group: modelData
                                noteCount: members.length
                                lastModified: members.length > 0 ? members[0].modified : null
                                onClicked: sidebar.section = "group:" + modelData.id
                                onContextMenuRequested: page.showGroupMenu(modelData)
                            }
                        }
                    }
                }

                // Notes
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 36
                    Layout.rightMargin: 36
                    Layout.bottomMargin: 40
                    spacing: 14

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gapM

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Notes")
                            color: Theme.text
                            font.pixelSize: Theme.fontSection
                            font.weight: Font.DemiBold
                        }

                        AppButton {
                            visible: page.section === "trash" && page.visibleNotes.length > 0
                            implicitHeight: 36
                            danger: true
                            text: qsTr("Empty trash")
                            onClicked: emptyTrashDialog.open()
                        }

                        SortButton {
                            sortBy: page.sortBy
                            onSortByChanged: page.sortBy = sortBy
                        }

                        ViewToggle {}
                    }

                    ResponsiveGrid {
                        Layout.fillWidth: true
                        minCellWidth: 168
                        columnSpacing: 20
                        rowSpacing: Theme.gapXl

                        Repeater {
                            model: page.visibleNotes

                            delegate: NoteCard {
                                required property var modelData

                                width: parent.cellWidth
                                note: modelData
                                onClicked: page.openNote(modelData)
                            }
                        }
                    }

                    Label {
                        visible: page.section === "trash" && page.visibleNotes.length > 0
                        text: qsTr("Notes in the trash are deleted for good after 30 days.")
                        color: Theme.textMuted
                        font.pixelSize: 13
                    }

                    Label {
                        visible: page.visibleNotes.length === 0
                        Layout.topMargin: Theme.gapXl
                        Layout.alignment: Qt.AlignHCenter
                        text: !Library.ready ? ""
                            : page.section === "trash" ? qsTr("The trash is empty.")
                            : page.query !== "" ? qsTr("No notes match “%1”.").arg(search.text.trim())
                            : page.section === "all" ? qsTr("No notes yet. Start one with New note.")
                            : qsTr("No notes here yet.")
                        color: Theme.textMuted
                    }
                }
            }
        }
    }

    // Context menus and dialogs

    property var menuNote: ({})
    property var menuGroup: ({})

    function openNewNote() {
        newNoteDialog.preselectedGroup = page.groupId
        newNoteDialog.open()
    }

    function showNoteMenu(note) {
        menuNote = note
        noteMenu.popup()
    }

    function showGroupMenu(group) {
        menuGroup = group
        groupMenu.popup()
    }

    NoteMenu {
        id: noteMenu

        note: page.menuNote
        onOpenRequested: page.openNote(page.menuNote.id)
        onRenameRequested: {
            renameNoteDialog.text = page.menuNote.title
            renameNoteDialog.open()
        }
        onDeleteForeverRequested: deleteNoteDialog.open()
    }

    GroupMenu {
        id: groupMenu

        group: page.menuGroup
        onRenameRequested: {
            renameGroupDialog.text = page.menuGroup.name
            renameGroupDialog.open()
        }
        onDeleteRequested: deleteGroupDialog.open()
    }

    TextPromptDialog {
        id: renameNoteDialog

        title: qsTr("Rename note")
        confirmText: qsTr("Rename")
        placeholderText: qsTr("Title")
        onSubmitted: text => Library.renameNote(page.menuNote.id, text)
    }

    TextPromptDialog {
        id: renameGroupDialog

        title: qsTr("Rename group")
        confirmText: qsTr("Rename")
        placeholderText: qsTr("Group name")
        onSubmitted: text => Library.renameGroup(page.menuGroup.id, text)
    }

    NewNoteDialog {
        id: newNoteDialog

        onCreated: noteId => page.openNote(noteId)
    }

    GroupDialog {
        id: groupDialog

        onSubmitted: (name, color) => sidebar.section = "group:" + Library.createGroup(name, color)
    }

    ConfirmDialog {
        id: deleteNoteDialog

        title: qsTr("Delete forever?")
        message: qsTr("“%1” will be deleted for good. This can't be undone.").arg(page.menuNote.title ?? "")
        confirmText: qsTr("Delete")
        onAccepted: Library.deleteNoteForever(page.menuNote.id)
    }

    ConfirmDialog {
        id: deleteGroupDialog

        title: qsTr("Delete group?")
        message: qsTr("The group “%1” will be deleted. Its notes become standalone notes.").arg(page.menuGroup.name ?? "")
        confirmText: qsTr("Delete group")
        onAccepted: {
            if (page.section === "group:" + page.menuGroup.id)
                sidebar.section = "all"
            Library.deleteGroup(page.menuGroup.id)
        }
    }

    ConfirmDialog {
        id: emptyTrashDialog

        title: qsTr("Empty the trash?")
        message: qsTr("%1 will be deleted for good. This can't be undone.").arg(Format.noteCount(page.visibleNotes.length))
        confirmText: qsTr("Empty trash")
        onAccepted: Library.emptyTrash()
    }
}
