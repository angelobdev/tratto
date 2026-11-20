// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Editor: top bar, tool row, the page area and the pages panel.
// The canvas shows one page at a time; the pages panel switches between them.
Page {
    id: page

    required property string noteId

    signal back()

    property bool pagesPanelOpen: true

    // Saves what is pending before leaving; the session also saves on destruction.
    function leave() {
        session.flush()
        page.back()
    }

    NoteSession {
        id: session

        Component.onCompleted: open(page.noteId)
        onLoadFailed: page.back()
        // Keep the canvas on an existing page when pages are removed (undo of Add page).
        onPageCountChanged: if (canvas.pageIndex >= pageCount) canvas.pageIndex = Math.max(0, pageCount - 1)
    }

    EditorTools { id: tools }

    background: Rectangle { color: Theme.canvas }

    // Tool shortcuts. Single letters: text fields still receive typed letters.
    Shortcut { sequence: "P"; onActivated: tools.tool = EditorTools.Pen }
    Shortcut { sequence: "H"; onActivated: tools.tool = EditorTools.Highlighter }
    Shortcut { sequence: "E"; onActivated: tools.tool = EditorTools.Eraser }
    Shortcut { sequence: "T"; onActivated: tools.tool = EditorTools.Text }
    Shortcut { sequence: "S"; onActivated: tools.tool = EditorTools.Select }
    Shortcut { sequence: "I"; onActivated: tools.tool = EditorTools.Image }
    Shortcut { sequences: [StandardKey.ZoomIn, "Ctrl+="]; onActivated: canvas.zoomIn() }
    Shortcut { sequences: [StandardKey.ZoomOut]; onActivated: canvas.zoomOut() }
    Shortcut { sequences: [StandardKey.Undo]; onActivated: session.undo() }
    Shortcut { sequences: [StandardKey.Redo, "Ctrl+Shift+Z"]; onActivated: session.redo() }

    header: ColumnLayout {
        spacing: 0

        EditorTopBar {
            Layout.fillWidth: true
            session: session
            pagesPanelOpen: page.pagesPanelOpen
            onBack: page.leave()
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

            CanvasItem {
                id: canvas

                anchors.fill: parent
                session: session
                tools: tools
                frameColor: Theme.border
            }

            ZoomControl {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: Theme.gapL
                zoom: canvas.zoom
                canZoomIn: canvas.canZoomIn
                canZoomOut: canvas.canZoomOut
                currentPage: canvas.pageIndex
                pageCount: session.pageCount
                onZoomInRequested: canvas.zoomIn()
                onZoomOutRequested: canvas.zoomOut()
            }
        }

        PagesPanel {
            visible: page.pagesPanelOpen
            Layout.fillHeight: true
            pages: session.pages
            currentPage: canvas.pageIndex
            onPageClicked: index => canvas.pageIndex = index
            onAddPageRequested: {
                session.addPage()
                canvas.pageIndex = session.pageCount - 1
            }
        }
    }
}
