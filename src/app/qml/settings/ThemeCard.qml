// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Theme choice with a miniature of the app in that theme.
// `scheme` is "light", "dark" or "system" (half light, half dark).
AbstractButton {
    id: card

    property string scheme: "light"

    checkable: true
    hoverEnabled: true
    padding: 10
    Accessible.role: Accessible.RadioButton
    Accessible.name: text

    // Miniature colors are fixed: they show each theme, not the current one.
    component Miniature: Rectangle {
        id: mini

        property bool darkTheme: false
        property bool withSidebar: true

        color: darkTheme ? "#111113" : "#F4F4F5"

        RowLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                visible: mini.withSidebar
                Layout.fillHeight: true
                Layout.preferredWidth: mini.width * 0.3
                color: mini.darkTheme ? "#1C1C1F" : "#E4E4E7"
            }

            ColumnLayout {
                id: body

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 10
                spacing: 6

                Rectangle {
                    Layout.preferredHeight: 8
                    Layout.preferredWidth: body.width * (mini.withSidebar ? 0.5 : 0.6)
                    radius: 2
                    color: mini.darkTheme ? "#F4F4F5" : "#18181B"
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 4
                    color: mini.darkTheme ? "#2A2A2F" : "#FFFFFF"
                }
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 10

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 92
            clip: true

            Rectangle {
                id: frame

                anchors.fill: parent
                radius: 8
                color: "transparent"
                border.color: card.scheme === "dark" ? "#2E2E33" : "transparent"
                z: 1
            }

            Miniature {
                visible: card.scheme !== "system"
                anchors.fill: parent
                radius: 8
                darkTheme: card.scheme === "dark"
            }

            Row {
                visible: card.scheme === "system"
                anchors.fill: parent

                // Both halves are rounded; a strip at the inner edge squares off the seam.
                Miniature {
                    width: parent.width / 2
                    height: parent.height
                    withSidebar: false
                    radius: 8

                    Rectangle { anchors.right: parent.right; width: 8; height: parent.height; color: parent.color }
                }

                Miniature {
                    width: parent.width / 2
                    height: parent.height
                    withSidebar: false
                    darkTheme: true
                    radius: 8

                    Rectangle { anchors.left: parent.left; width: 8; height: parent.height; color: parent.color }
                }
            }
        }

        Label {
            Layout.leftMargin: 4
            Layout.bottomMargin: 4
            text: card.text
            color: Theme.text
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }
    }

    background: Rectangle {
        radius: 14
        color: card.hovered && !card.checked ? Theme.surfaceAlt : Theme.surface
        border.width: card.checked ? 2 : 1
        border.color: card.checked ? Theme.accentInk : Theme.border

        FocusRing { visible: card.visualFocus }
    }
}
