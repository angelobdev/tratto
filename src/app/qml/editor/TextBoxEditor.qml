// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

// Editor for the text box the canvas is editing: placed over the box, styled from
// the toolbar, and committed when it loses focus, on Esc, or when the canvas asks.
TextEdit {
    id: editor

    required property CanvasItem canvas
    required property EditorTools tools

    function commit() {
        if (canvas.editingText)
            canvas.commitText(text)
    }

    visible: canvas.editingText
    x: canvas.textEditRect.x
    y: canvas.textEditRect.y
    width: canvas.textEditRect.width
    // The same families the renderer uses (render::fontFamilies), so the text
    // doesn't jump when it is committed.
    font.family: tools.textFont === EditorTools.Serif ? "serif"
               : tools.textFont === EditorTools.Monospace ? "monospace"
               : Qt.application.font.family
    font.pixelSize: Math.max(1, Math.round(tools.textSize * canvas.textScale))
    font.bold: tools.textBold
    font.italic: tools.textItalic
    font.hintingPreference: Font.PreferNoHinting
    color: tools.penColors[tools.textColor] ?? "black"
    selectionColor: Theme.accentSoft
    selectedTextColor: color
    wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
    selectByMouse: true
    persistentSelection: true
    Accessible.name: qsTr("Text")

    onActiveFocusChanged: if (!activeFocus) commit()
    Keys.onEscapePressed: commit()

    Connections {
        target: editor.canvas
        function onTextEditStarted() {
            editor.text = editor.canvas.textEditInitial
            editor.forceActiveFocus()
            editor.cursorPosition = editor.length
        }
        function onTextCommitRequested() { editor.commit() }
    }

    // Accent ring around the box being edited (as in the design).
    Rectangle {
        anchors.fill: parent
        anchors.margins: -6
        radius: 4
        z: -1
        color: "transparent"
        border.width: 1.5
        border.color: Theme.accentInk
    }
}
