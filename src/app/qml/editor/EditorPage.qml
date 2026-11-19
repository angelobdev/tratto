// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts

// Editor: top bar, tool row, the page area and the pages panel.
// The page shows only its paper for now; the canvas replaces it in M1.
Page {
    id: page

    // { title, group, paper: { pattern, spacingMm, color } }, see FakeLibrary.
    required property var note

    signal back()

    // 100% zoom shows an A4 page 680 px wide, as in the design.
    readonly property real pixelsPerPointAt100: 680 / 595.28
    readonly property var zoomLevels: [0.5, 0.75, 1, 1.5, 2, 3, 4]
    property int zoomIndex: 2
    readonly property real zoom: zoomLevels[zoomIndex]

    // Placeholder pages until notes are stored (M2): three pages of the note's paper.
    readonly property var pages: [note.paper, note.paper, note.paper]
    property int currentPage: 0
    property bool pagesPanelOpen: true

    EditorTools { id: tools }

    background: Rectangle { color: Theme.canvas }

    // Tool shortcuts. Single letters: text fields still receive typed letters.
    Shortcut { sequence: "P"; onActivated: tools.tool = EditorTools.Pen }
    Shortcut { sequence: "H"; onActivated: tools.tool = EditorTools.Highlighter }
    Shortcut { sequence: "E"; onActivated: tools.tool = EditorTools.Eraser }
    Shortcut { sequence: "T"; onActivated: tools.tool = EditorTools.Text }
    Shortcut { sequence: "S"; onActivated: tools.tool = EditorTools.Select }
    Shortcut { sequence: "I"; onActivated: tools.tool = EditorTools.Image }
    Shortcut { sequences: [StandardKey.ZoomIn, "Ctrl+="]; onActivated: page.zoomIn() }
    Shortcut { sequences: [StandardKey.ZoomOut]; onActivated: page.zoomOut() }

    function zoomIn() { zoomIndex = Math.min(zoomIndex + 1, zoomLevels.length - 1) }
    function zoomOut() { zoomIndex = Math.max(zoomIndex - 1, 0) }

    header: ColumnLayout {
        spacing: 0

        EditorTopBar {
            Layout.fillWidth: true
            note: page.note
            pagesPanelOpen: page.pagesPanelOpen
            onBack: page.back()
            onTogglePagesPanel: page.pagesPanelOpen = !page.pagesPanelOpen
        }

        ToolRow {
            Layout.fillWidth: true
            tools: tools
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Flickable {
                id: pageArea

                anchors.fill: parent
                contentWidth: Math.max(width, sheet.width + 48)
                contentHeight: sheet.height + 32 + 96
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar {}
                ScrollBar.horizontal: ScrollBar {}

                // Soft page shadow (DESIGN.md: 0 10px 30px rgba(24,24,27,.08)).
                MultiEffect {
                    source: sheetShape
                    anchors.fill: sheet
                    shadowEnabled: true
                    shadowColor: Qt.rgba(0.094, 0.094, 0.106, Theme.dark ? 0.5 : 0.08)
                    shadowVerticalOffset: 10
                    shadowBlur: 1.0
                    blurMax: 30
                    autoPaddingEnabled: true
                }

                Rectangle {
                    id: sheetShape

                    anchors.fill: sheet
                    visible: false
                    color: "white"
                }

                PaperThumbnail {
                    id: sheet

                    x: (pageArea.contentWidth - width) / 2
                    y: 32
                    width: 595.28 * page.pixelsPerPointAt100 * page.zoom
                    height: 841.89 * page.pixelsPerPointAt100 * page.zoom
                    pattern: page.pages[page.currentPage].pattern
                    spacingMm: page.pages[page.currentPage].spacingMm
                    paperColor: page.pages[page.currentPage].color

                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.color: Qt.rgba(0.094, 0.094, 0.106, 0.05)
                    }
                }
            }

            ZoomControl {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: Theme.gapL
                zoom: page.zoom
                canZoomIn: page.zoomIndex < page.zoomLevels.length - 1
                canZoomOut: page.zoomIndex > 0
                currentPage: page.currentPage
                pageCount: page.pages.length
                onZoomInRequested: page.zoomIn()
                onZoomOutRequested: page.zoomOut()
            }
        }

        PagesPanel {
            visible: page.pagesPanelOpen
            Layout.fillHeight: true
            pages: page.pages
            currentPage: page.currentPage
            onPageClicked: index => page.currentPage = index
        }
    }
}
