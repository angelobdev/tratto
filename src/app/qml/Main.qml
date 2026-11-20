// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    width: 1280
    height: 800
    minimumWidth: 720
    minimumHeight: 480
    visible: true
    title: "Tratto"
    color: Theme.background

    font.pixelSize: Theme.fontBody

    // The Basic style draws its controls from this palette, so they follow the theme.
    palette {
        window: Theme.background
        windowText: Theme.text
        base: Theme.surface
        alternateBase: Theme.surfaceAlt
        text: Theme.text
        button: Theme.surfaceAlt
        buttonText: Theme.text
        brightText: Theme.accentForeground
        highlight: Theme.accent
        highlightedText: Theme.accentForeground
        placeholderText: Theme.textMuted
        light: Theme.surface
        midlight: Theme.surfaceAlt
        mid: Theme.borderStrong
        dark: Theme.borderStrong
        shadow: Theme.border
        toolTipBase: Theme.surface
        toolTipText: Theme.text
    }

    // Page to show at startup: "library", "settings", "editor" or "new-note" (--page, for development).
    property string startPage: "library"

    StackView {
        id: stack

        objectName: "stack"
        anchors.fill: parent
        initialItem: libraryPage
        focus: true

        Component.onCompleted: {
            if (window.startPage === "settings")
                stack.push(settingsPage, {}, StackView.Immediate)
            else if (window.startPage === "new-note")
                stack.currentItem.openNewNote()
        }

        // --page editor opens the most recent note once the library is loaded.
        Connections {
            target: Library
            enabled: window.startPage === "editor"
            function onReadyChanged() {
                const note = Library.notes.find(n => !n.inTrash)
                if (note)
                    stack.push(editorPage, { noteId: note.id }, StackView.Immediate)
            }
        }
    }

    Component {
        id: libraryPage

        LibraryPage {
            onOpenNote: noteId => stack.push(editorPage, { noteId: noteId })
            onOpenSettings: stack.push(settingsPage)
        }
    }

    Component {
        id: settingsPage

        SettingsPage {
            focus: true
            onBack: stack.pop()
        }
    }

    Component {
        id: editorPage

        EditorPage {
            onBack: stack.pop()
        }
    }
}
