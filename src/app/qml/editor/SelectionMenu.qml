// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Dark pill with the selection actions, shown under (or above) the selection.
// Same look in both themes, as in the design.
Rectangle {
    id: menu

    required property CanvasItem canvas
    required property EditorTools tools

    readonly property rect target: canvas.selectionRect
    visible: canvas.hasSelection && !canvas.selectionBusy
    x: Math.max(8, Math.min(target.x, parent.width - width - 8))
    y: target.y + target.height + 8 + height <= parent.height - 8
       ? target.y + target.height + 8
       : Math.max(8, target.y - height - 8)
    implicitWidth: row.implicitWidth + 8
    implicitHeight: row.implicitHeight + 8
    radius: Theme.radiusCard
    color: "#18181B"

    component Action: AbstractButton {
        id: action

        property color textColor: "#FFFFFF"

        implicitHeight: 40
        leftPadding: 12
        rightPadding: 12
        hoverEnabled: true

        contentItem: Label {
            text: action.text
            verticalAlignment: Text.AlignVCenter
            color: action.textColor
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        background: Rectangle {
            radius: 8
            color: action.down ? Qt.rgba(1, 1, 1, 0.16) : action.hovered ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
        }
    }

    // Picking a color replaces the actions inside the same pill: everything stays in
    // the menu's area, where the stylus reaches the menu instead of the canvas.
    property bool pickingColor: false

    Connections {
        target: menu.canvas
        function onSelectionChanged() {
            if (!menu.canvas.hasSelection || menu.canvas.selectionBusy)
                menu.pickingColor = false
        }
    }

    RowLayout {
        id: row

        anchors.centerIn: parent
        spacing: 2

        Action {
            visible: !menu.pickingColor
            text: qsTr("Copy")
            onClicked: menu.canvas.copySelection()
        }
        Action {
            visible: !menu.pickingColor
            text: qsTr("Cut")
            onClicked: menu.canvas.cutSelection()
        }
        Action {
            visible: !menu.pickingColor
            text: qsTr("Duplicate")
            onClicked: menu.canvas.duplicateSelection()
        }
        Action {
            objectName: "selectionColor"
            visible: !menu.pickingColor
            text: qsTr("Color")
            onClicked: menu.pickingColor = true
        }
        Action {
            objectName: "selectionDelete"
            visible: !menu.pickingColor
            text: qsTr("Delete")
            textColor: "#FFA8A8"
            onClicked: menu.canvas.deleteSelection()
        }

        // Color choices
        AbstractButton {
            visible: menu.pickingColor
            implicitWidth: 36
            implicitHeight: 40
            hoverEnabled: true
            text: qsTr("Back")
            Accessible.name: text
            onClicked: menu.pickingColor = false

            contentItem: Item {
                Icon {
                    anchors.centerIn: parent
                    name: "chevron-left"
                    size: 18
                    strokeWidth: 2
                    color: "#FFFFFF"
                }
            }
            background: Rectangle {
                radius: 8
                color: parent.hovered ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
            }
        }

        Repeater {
            model: menu.pickingColor ? menu.tools.penColors : []

            delegate: AbstractButton {
                id: swatch

                required property color modelData
                required property int index

                objectName: "selectionSwatch" + index
                implicitWidth: 36
                implicitHeight: 40
                hoverEnabled: true
                Accessible.name: modelData.toString()
                onClicked: {
                    menu.canvas.recolorSelection(modelData)
                    menu.pickingColor = false
                }

                contentItem: Item {}
                background: Item {
                    Rectangle {
                        anchors.centerIn: parent
                        width: swatch.hovered ? 26 : 22
                        height: width
                        radius: width / 2
                        color: swatch.modelData
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.3)
                    }
                }
            }
        }
    }
}
