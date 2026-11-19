// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Library: sidebar navigation, header with search and actions, groups and notes.
Page {
    id: page

    signal openNote(var note)
    signal openSettings()

    background: Rectangle { color: Theme.background }

    // "recent" = modified in the last few days
    readonly property int recentDays: 3
    property string sortBy: "modified" // "modified" | "title"

    readonly property string section: sidebar.section
    readonly property string groupId: section.startsWith("group:") ? section.slice(6) : ""
    readonly property string query: search.text.trim().toLowerCase()

    readonly property var visibleNotes: {
        let notes = FakeLibrary.notes
        if (section === "trash")
            notes = []
        else if (section === "recent")
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
        default: return FakeLibrary.group(groupId)?.name ?? ""
        }
    }

    readonly property string subtitle: section === "all"
        ? Format.noteCount(visibleNotes.length) + " · " + Format.groupCount(FakeLibrary.groups.length)
        : Format.noteCount(visibleNotes.length)

    RowLayout {
        anchors.fill: parent
        spacing: 0

        LibrarySidebar {
            id: sidebar

            Layout.fillHeight: true
            onSettingsRequested: page.openSettings()
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
                            onClicked: page.openNote(FakeLibrary.newNote())
                        }
                    }
                }

                // Groups (only in "All notes", when not searching)
                ColumnLayout {
                    visible: page.section === "all" && page.query === ""
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
                            model: FakeLibrary.groups

                            delegate: GroupCard {
                                required property var modelData

                                width: parent.cellWidth
                                group: modelData
                                noteCount: FakeLibrary.notesInGroup(modelData.id).length
                                lastModified: FakeLibrary.lastModifiedInGroup(modelData.id)
                                onClicked: sidebar.section = "group:" + modelData.id
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
                        visible: page.visibleNotes.length === 0
                        Layout.topMargin: Theme.gapXl
                        Layout.alignment: Qt.AlignHCenter
                        text: page.section === "trash" ? qsTr("The trash is empty.")
                            : page.query !== "" ? qsTr("No notes match “%1”.").arg(search.text.trim())
                            : qsTr("No notes here yet.")
                        color: Theme.textMuted
                    }
                }
            }
        }
    }
}
