// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Live preview of the accent on a button, the active tool, a switch and a link.
// Decorative: none of these elements is interactive.
Rectangle {
    implicitHeight: content.implicitHeight + 36
    radius: 14
    color: Theme.surface
    border.color: Theme.border

    ColumnLayout {
        id: content

        anchors.fill: parent
        anchors.margins: 18
        spacing: Theme.gapL

        Label {
            text: qsTr("Preview").toUpperCase()
            color: Theme.textMuted
            font.pixelSize: Theme.fontLabel
            font.weight: Font.DemiBold
            font.letterSpacing: Theme.fontLabel * 0.06
        }

        Flow {
            Layout.fillWidth: true
            spacing: Theme.gapL

            Rectangle {
                width: buttonRow.implicitWidth + 32
                height: 42
                radius: Theme.radiusControl
                color: Theme.accent

                Row {
                    id: buttonRow

                    anchors.centerIn: parent
                    spacing: Theme.gapS

                    Icon {
                        anchors.verticalCenter: parent.verticalCenter
                        name: "plus"
                        color: Theme.accentForeground
                        strokeWidth: 2
                    }

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("New note")
                        color: Theme.accentForeground
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }
                }
            }

            Rectangle {
                width: 44
                height: 44
                radius: Theme.radiusControl
                color: Theme.accentSoft

                Icon {
                    anchors.centerIn: parent
                    name: "pen"
                    size: 22
                    color: Theme.accentInk
                }
            }

            Item {
                width: previewSwitch.implicitWidth
                height: 44

                AppSwitch {
                    id: previewSwitch

                    anchors.verticalCenter: parent.verticalCenter
                    checked: true
                }
            }

            Label {
                height: 44
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Link")
                color: Theme.accentInk
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("In dark mode, text and icons use a lighter version of the color so they stay readable.")
            wrapMode: Text.Wrap
            color: Theme.textMuted
            font.pixelSize: 12
        }
    }
}
