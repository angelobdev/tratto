// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Tool row: the six tools, then the options of the current tool.
// Centered; scrolls sideways when the window is too narrow.
Rectangle {
    id: bar

    required property EditorTools tools

    implicitHeight: 60
    color: Theme.surface

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.border
    }

    component ToolIconButton: IconButton {
        id: toolButton

        property int tool
        property color inkColor: "transparent" // current ink color, under the icon

        size: Theme.controlHeight
        radius: Theme.radiusControl
        iconSize: 22
        iconStrokeWidth: 1.8
        active: bar.tools.tool === tool
        Accessible.checkable: true
        Accessible.checked: active
        onClicked: bar.tools.tool = tool

        Rectangle {
            visible: toolButton.inkColor.a > 0
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 5
            anchors.horizontalCenter: parent.horizontalCenter
            width: 18
            height: 3
            radius: 2
            color: toolButton.inkColor
        }
    }

    // Color swatch: a round dot (pen, text) or a wide bar (highlighter).
    component ColorChoice: AbstractButton {
        id: choice

        property color color
        property bool current: false
        property bool wide: false

        implicitWidth: wide ? 36 : 34
        implicitHeight: Theme.controlHeight
        hoverEnabled: true
        checkable: true
        checked: current
        Accessible.role: Accessible.RadioButton
        ToolTip.visible: hovered
        ToolTip.text: text
        ToolTip.delay: 600

        contentItem: Item {}

        background: Item {
            Rectangle {
                visible: choice.current || choice.visualFocus
                anchors.centerIn: parent
                width: swatch.width + 8
                height: swatch.height + 8
                radius: swatch.radius + 4
                color: "transparent"
                border.width: 2
                border.color: Theme.accentInk
            }

            Rectangle {
                id: swatch

                anchors.centerIn: parent
                width: choice.wide ? 24 : 22
                height: choice.wide ? 16 : 22
                radius: choice.wide ? 5 : 11
                color: choice.color
                // Keeps dark swatches visible on the dark theme.
                border.width: 1
                border.color: Theme.dark ? Qt.rgba(1, 1, 1, 0.25) : Qt.rgba(0.094, 0.094, 0.106, 0.15)
            }
        }
    }

    // Size choice: 40 x 44, the preview item shows the size.
    component SizeChoice: AbstractButton {
        id: sizeChoice

        property bool current: false
        default property alias preview: holder.data

        implicitWidth: 40
        implicitHeight: Theme.controlHeight
        hoverEnabled: true
        checkable: true
        checked: current
        Accessible.role: Accessible.RadioButton
        ToolTip.visible: hovered
        ToolTip.text: text
        ToolTip.delay: 600

        contentItem: Item { id: holder }

        background: Rectangle {
            radius: Theme.radiusControl
            color: sizeChoice.current ? Theme.accentSoft
                 : sizeChoice.hovered ? Theme.hover : "transparent"

            FocusRing { visible: sizeChoice.visualFocus }
        }
    }

    readonly property var penColorNames: [
        qsTr("Black"), qsTr("Blue"), qsTr("Red"), qsTr("Green"), qsTr("Orange"), qsTr("Violet"),
    ]
    readonly property var highlighterColorNames: [
        qsTr("Yellow"), qsTr("Green"), qsTr("Pink"), qsTr("Sky"), qsTr("Orange"),
    ]
    readonly property color penColor: tools.penColors[tools.penColor]
    readonly property color highlighterColor: tools.highlighterColors[tools.highlighterColor]

    Flickable {
        id: flick

        anchors.fill: parent
        contentWidth: Math.max(width, row.implicitWidth + 24)
        contentHeight: height
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        RowLayout {
            id: row

            x: Math.max(12, (flick.width - implicitWidth) / 2)
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            ToolIconButton {
                tool: EditorTools.Pen
                iconName: "pen"
                text: qsTr("Pen (P)")
                inkColor: bar.penColor
            }
            ToolIconButton {
                tool: EditorTools.Highlighter
                iconName: "highlighter"
                text: qsTr("Highlighter (H)")
                inkColor: bar.highlighterColor
            }
            ToolIconButton { tool: EditorTools.Eraser; iconName: "eraser"; text: qsTr("Eraser (E)") }
            ToolIconButton { tool: EditorTools.Text; iconName: "type"; text: qsTr("Text (T)") }
            ToolIconButton { tool: EditorTools.Select; iconName: "lasso"; text: qsTr("Selection (S)") }
            ToolIconButton { tool: EditorTools.Image; iconName: "image"; text: qsTr("Insert image (I)") }

            Divider { Layout.leftMargin: 8; Layout.rightMargin: 8 }

            // Pen options
            RowLayout {
                visible: bar.tools.tool === EditorTools.Pen
                spacing: 0

                Repeater {
                    model: bar.tools.penColors

                    delegate: ColorChoice {
                        required property color modelData
                        required property int index

                        color: modelData
                        text: bar.penColorNames[index] ?? ""
                        current: bar.tools.penColor === index
                        onClicked: bar.tools.penColor = index
                    }
                }

                // Custom ink colors arrive with the canvas.
                AbstractButton {
                    implicitWidth: 34
                    implicitHeight: Theme.controlHeight
                    enabled: false
                    opacity: 0.4
                    text: qsTr("More colors")
                    Accessible.name: text

                    contentItem: Item {
                        Rectangle {
                            anchors.centerIn: parent
                            width: 22
                            height: 22
                            radius: 11
                            color: "transparent"
                            border.width: 1.5
                            border.color: Theme.textMuted

                            Icon {
                                anchors.centerIn: parent
                                name: "plus"
                                size: 12
                                strokeWidth: 3
                            }
                        }
                    }
                }

                Divider {}

                Repeater {
                    model: [
                        { diameter: 4, label: qsTr("Fine stroke") },
                        { diameter: 8, label: qsTr("Medium stroke") },
                        { diameter: 13, label: qsTr("Thick stroke") },
                    ]

                    delegate: SizeChoice {
                        required property var modelData
                        required property int index

                        text: modelData.label
                        current: bar.tools.penWidth === index
                        onClicked: bar.tools.penWidth = index

                        Rectangle {
                            anchors.centerIn: parent
                            width: modelData.diameter
                            height: width
                            radius: width / 2
                            color: bar.penColor
                        }
                    }
                }
            }

            // Highlighter options
            RowLayout {
                visible: bar.tools.tool === EditorTools.Highlighter
                spacing: 0

                Repeater {
                    model: bar.tools.highlighterColors

                    delegate: ColorChoice {
                        required property color modelData
                        required property int index

                        wide: true
                        color: modelData
                        text: bar.highlighterColorNames[index] ?? ""
                        current: bar.tools.highlighterColor === index
                        onClicked: bar.tools.highlighterColor = index
                    }
                }

                Divider {}

                Repeater {
                    model: [
                        { height: 6, label: qsTr("Fine highlighter") },
                        { height: 10, label: qsTr("Medium highlighter") },
                        { height: 15, label: qsTr("Thick highlighter") },
                    ]

                    delegate: SizeChoice {
                        required property var modelData
                        required property int index

                        text: modelData.label
                        current: bar.tools.highlighterWidth === index
                        onClicked: bar.tools.highlighterWidth = index

                        Rectangle {
                            anchors.centerIn: parent
                            width: 22
                            height: modelData.height
                            radius: 3
                            color: bar.highlighterColor
                        }
                    }
                }

                Divider {}

                CheckOption {
                    text: qsTr("Straight line")
                    checked: bar.tools.straightHighlighter
                    onToggled: bar.tools.straightHighlighter = checked
                }
            }

            // Eraser options
            RowLayout {
                visible: bar.tools.tool === EditorTools.Eraser
                spacing: 0

                SegmentedChoice {
                    value: bar.tools.eraserMode
                    options: [
                        { value: EditorTools.WholeStroke, label: qsTr("Whole stroke") },
                        { value: EditorTools.Partial, label: qsTr("Partial") },
                    ]
                    onPicked: value => bar.tools.eraserMode = value
                }

                Divider { visible: bar.tools.eraserMode === EditorTools.Partial }

                Repeater {
                    model: bar.tools.eraserMode === EditorTools.Partial ? [
                        { diameter: 10, label: qsTr("Small eraser") },
                        { diameter: 16, label: qsTr("Medium eraser") },
                        { diameter: 24, label: qsTr("Large eraser") },
                    ] : []

                    delegate: SizeChoice {
                        required property var modelData
                        required property int index

                        text: modelData.label
                        current: bar.tools.eraserSize === index
                        onClicked: bar.tools.eraserSize = index

                        Rectangle {
                            anchors.centerIn: parent
                            width: modelData.diameter
                            height: width
                            radius: width / 2
                            color: "transparent"
                            border.width: 1.5
                            border.color: Theme.textSecondary
                        }
                    }
                }

                Divider {}

                CheckOption {
                    text: qsTr("Highlighter only")
                    checked: bar.tools.eraseHighlighterOnly
                    onToggled: bar.tools.eraseHighlighterOnly = checked
                }
            }

            // Text options
            RowLayout {
                visible: bar.tools.tool === EditorTools.Text
                spacing: 6

                CompactComboBox {
                    Accessible.name: qsTr("Font")
                    model: [
                        { value: EditorTools.Sans, text: "Inter" },
                        { value: EditorTools.Serif, text: qsTr("Serif") },
                        { value: EditorTools.Monospace, text: qsTr("Monospace") },
                    ]
                    value: bar.tools.textFont
                    onPicked: value => bar.tools.textFont = value
                }

                CompactComboBox {
                    Accessible.name: qsTr("Size")
                    model: bar.tools.textSizes.map(size => ({ value: size, text: String(size) }))
                    value: bar.tools.textSize
                    onPicked: value => bar.tools.textSize = value
                }

                SizeChoice {
                    text: qsTr("Bold")
                    current: bar.tools.textBold
                    onClicked: bar.tools.textBold = !bar.tools.textBold

                    Label {
                        anchors.centerIn: parent
                        text: "B"
                        color: Theme.text
                        font.pixelSize: 15
                        font.weight: Font.ExtraBold
                    }
                }

                SizeChoice {
                    text: qsTr("Italic")
                    current: bar.tools.textItalic
                    onClicked: bar.tools.textItalic = !bar.tools.textItalic

                    Label {
                        anchors.centerIn: parent
                        text: "I"
                        color: Theme.text
                        font.family: "serif"
                        font.pixelSize: 15
                        font.italic: true
                        font.weight: Font.DemiBold
                    }
                }

                Divider {}

                Row {
                    Repeater {
                        model: bar.tools.penColors

                        delegate: ColorChoice {
                            required property color modelData
                            required property int index

                            color: modelData
                            text: bar.penColorNames[index] ?? ""
                            current: bar.tools.textColor === index
                            onClicked: bar.tools.textColor = index
                        }
                    }
                }
            }

            // Selection options
            RowLayout {
                visible: bar.tools.tool === EditorTools.Select
                spacing: 0

                SegmentedChoice {
                    value: bar.tools.selectionMode
                    options: [
                        { value: EditorTools.Lasso, label: qsTr("Freeform") },
                        { value: EditorTools.Rectangle, label: qsTr("Rectangle") },
                    ]
                    onPicked: value => bar.tools.selectionMode = value
                }

                Label {
                    leftPadding: 10
                    rightPadding: 10
                    text: qsTr("Drag around strokes, text or images")
                    color: Theme.textMuted
                    font.pixelSize: 13
                }
            }

            // Image options (inserting images arrives in M3)
            RowLayout {
                visible: bar.tools.tool === EditorTools.Image
                spacing: 6

                AppButton {
                    implicitHeight: 40
                    leftPadding: 14
                    rightPadding: 14
                    font.pixelSize: 13
                    text: qsTr("From file…")
                    enabled: false
                    opacity: enabled ? 1 : 0.5
                }

                AppButton {
                    implicitHeight: 40
                    leftPadding: 14
                    rightPadding: 14
                    font.pixelSize: 13
                    text: qsTr("From clipboard")
                    enabled: false
                    opacity: enabled ? 1 : 0.5
                }

                Label {
                    leftPadding: 10
                    rightPadding: 10
                    text: qsTr("Or drag an image onto the page")
                    color: Theme.textMuted
                    font.pixelSize: 13
                }
            }
        }
    }
}
