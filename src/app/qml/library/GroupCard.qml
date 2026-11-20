// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Group card: a small stack of sheets in the group color, name and summary.
AbstractButton {
    id: control

    required property var group      // an entry of Library.groups
    required property int noteCount
    required property var lastModified

    signal contextMenuRequested()

    implicitHeight: 88
    onPressAndHold: contextMenuRequested()

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: control.contextMenuRequested()
    }
    padding: 14
    hoverEnabled: true
    Accessible.name: group.name

    // Tinted back sheet: a pale group color in light mode, a translucent one in dark mode.
    readonly property color tint: Theme.dark
        ? Qt.rgba(group.color.r, group.color.g, group.color.b, 0.22)
        : Appearance.mixWithWhite(group.color, 0.85)
    readonly property color tintBorder: Theme.dark
        ? Qt.rgba(group.color.r, group.color.g, group.color.b, 0.4)
        : Appearance.mixWithWhite(group.color, 0.7)

    contentItem: RowLayout {
        spacing: 14

        Item {
            implicitWidth: 48
            implicitHeight: 60

            Rectangle {
                anchors.fill: parent
                radius: 5
                rotation: -8
                antialiasing: true
                color: control.tint
                border.color: control.tintBorder
            }

            // Front sheet with a hint of the group's paper. Decorative, so it is drawn
            // with plain 1 px lines rather than at the real paper scale.
            Rectangle {
                anchors.fill: parent
                rotation: 4
                radius: 5
                antialiasing: true
                color: "#FFFFFF"
                border.color: Theme.border

                Item {
                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true

                    Repeater {
                        model: 8

                        Rectangle {
                            required property int index
                            y: 7 + index * 8
                            width: parent.width
                            height: 1
                            color: "#DCE3EE"
                        }
                    }

                    Repeater {
                        model: 0

                        Rectangle {
                            required property int index
                            x: 6 + index * 7
                            width: 1
                            height: parent.height
                            color: "#DCE3EE"
                        }
                    }
                }
            }

            Rectangle {
                x: -4
                y: parent.height - height + 4
                width: 14
                height: 14
                radius: 4
                color: control.group.color
                border.width: 2
                border.color: Theme.surface
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3

            Label {
                Layout.fillWidth: true
                text: control.group.name
                elide: Text.ElideRight
                color: Theme.text
                font.pixelSize: Theme.fontSection
                font.weight: Font.DemiBold
            }

            Label {
                Layout.fillWidth: true
                text: Format.noteCount(control.noteCount) + " · " + Format.updated(control.lastModified)
                elide: Text.ElideRight
                color: Theme.textMuted
                font.pixelSize: Theme.fontLabel
            }
        }
    }

    background: Rectangle {
        radius: Theme.radiusCard
        color: control.down ? Theme.surfaceAlt : Theme.surface
        border.color: control.hovered ? Theme.borderStrong : Theme.border

        FocusRing { visible: control.visualFocus }
    }
}
