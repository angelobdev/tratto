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
        brightText: Theme.onAccent
        highlight: Theme.accent
        highlightedText: Theme.onAccent
        placeholderText: Theme.textMuted
        light: Theme.surface
        midlight: Theme.surfaceAlt
        mid: Theme.borderStrong
        dark: Theme.borderStrong
        shadow: Theme.border
        toolTipBase: Theme.surface
        toolTipText: Theme.text
    }

    StackView {
        id: stack

        anchors.fill: parent
        initialItem: libraryPage
    }

    Component {
        id: libraryPage

        LibraryPage {
            onOpenNote: title => stack.push(editorPage, { noteTitle: title })
        }
    }

    Component {
        id: editorPage

        EditorPage {
            onBack: stack.pop()
        }
    }
}
