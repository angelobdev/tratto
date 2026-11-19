// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Pages panel: page thumbnails, the current one ringed, and "Add page".
Rectangle {
    id: panel

    required property var pages // list of paper styles { pattern, spacingMm, color }
    property int currentPage: 0

    signal pageClicked(int index)

    implicitWidth: 184
    color: Theme.surface

    Rectangle {
        width: 1
        height: parent.height
        color: Theme.border
    }

    ScrollView {
        id: scroll

        anchors.fill: parent
        anchors.leftMargin: 1
        contentWidth: availableWidth

        ColumnLayout {
            width: scroll.availableWidth
            spacing: Theme.gapL

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: Theme.gapL
                Layout.leftMargin: Theme.gapL
                Layout.rightMargin: Theme.gapL

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Pages")
                    color: Theme.text
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }

                Label {
                    text: panel.pages.length
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontLabel
                }
            }

            Repeater {
                model: panel.pages

                delegate: AbstractButton {
                    id: thumb

                    required property var modelData
                    required property int index
                    readonly property bool current: index === panel.currentPage

                    Layout.alignment: Qt.AlignHCenter
                    hoverEnabled: true
                    text: qsTr("Page %1").arg(index + 1)
                    Accessible.selected: current
                    onClicked: panel.pageClicked(index)

                    contentItem: ColumnLayout {
                        spacing: 6

                        Item {
                            implicitWidth: 120
                            implicitHeight: 170

                            // Selection ring: surface gap, then accent.
                            Rectangle {
                                visible: thumb.current || thumb.visualFocus
                                anchors.fill: parent
                                anchors.margins: -4
                                color: "transparent"
                                border.width: 2
                                border.color: Theme.accentInk
                            }

                            PaperThumbnail {
                                anchors.fill: parent
                                pattern: thumb.modelData.pattern
                                spacingMm: thumb.modelData.spacingMm
                                paperColor: thumb.modelData.color
                            }

                            Rectangle {
                                anchors.fill: parent
                                color: "transparent"
                                border.color: thumb.hovered && !thumb.current ? Theme.borderStrong : Theme.border
                            }
                        }

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: thumb.index + 1
                            color: thumb.current ? Theme.accentInk : Theme.textSecondary
                            font.pixelSize: Theme.fontLabel
                            font.weight: thumb.current ? Font.DemiBold : Font.Normal
                        }
                    }
                }
            }

            AbstractButton {
                id: addPage

                Layout.fillWidth: true
                Layout.leftMargin: Theme.gapL
                Layout.rightMargin: Theme.gapL
                Layout.bottomMargin: Theme.gapL
                implicitHeight: Theme.controlHeight
                text: qsTr("Add page")
                enabled: false // pages are added through commands in M2
                opacity: enabled ? 1 : 0.5

                contentItem: Item {
                    Row {
                        anchors.centerIn: parent
                        spacing: 6

                        Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            name: "plus"
                            size: 16
                            strokeWidth: 2
                        }

                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            text: addPage.text
                            color: Theme.textSecondary
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                    }
                }

                background: Rectangle {
                    radius: Theme.radiusControl
                    color: "transparent"
                    border.width: 1.5
                    border.color: Theme.borderStrong
                }
            }
        }
    }
}
