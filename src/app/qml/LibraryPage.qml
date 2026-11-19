// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Library skeleton with fake data. The real layout follows the approved designs
// in later M0 steps; the appearance controls here are temporary and move to the
// Settings dialog.
Page {
    id: page

    signal openNote(string title)

    background: Rectangle { color: Theme.background }

    readonly property var fakeNotes: [
        qsTr("Linear algebra"),
        qsTr("Weekly meeting"),
        qsTr("Trip ideas"),
        qsTr("Physics lab report"),
        qsTr("Reading list"),
    ]

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: Theme.sidebarWidth
            color: Theme.sidebar

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.gapM
                spacing: Theme.gapXs

                Repeater {
                    model: [
                        qsTr("All notes"),
                        qsTr("Recent"),
                        qsTr("Standalone notes"),
                        qsTr("Groups"),
                        qsTr("Trash"),
                    ]

                    delegate: ItemDelegate {
                        required property string modelData
                        required property int index

                        Layout.fillWidth: true
                        text: modelData
                        highlighted: index === 0
                        background: Rectangle {
                            radius: Theme.radiusControl
                            color: parent.highlighted ? Theme.accentSoft : "transparent"
                        }
                        contentItem: Label {
                            text: parent.text
                            color: parent.highlighted ? Theme.accentInk : Theme.textSecondary
                            font.weight: parent.highlighted ? Font.DemiBold : Font.Normal
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // Temporary appearance controls (move to Settings).
                Label {
                    text: qsTr("Appearance").toUpperCase()
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontLabel
                    font.weight: Font.DemiBold
                    font.letterSpacing: Theme.fontLabel * 0.06
                }

                ComboBox {
                    Layout.fillWidth: true
                    textRole: "text"
                    valueRole: "value"
                    model: [
                        { text: qsTr("System"), value: Appearance.System },
                        { text: qsTr("Light"), value: Appearance.Light },
                        { text: qsTr("Dark"), value: Appearance.Dark },
                    ]
                    Component.onCompleted: currentIndex = indexOfValue(Appearance.mode)
                    onActivated: Appearance.mode = currentValue
                }

                ComboBox {
                    Layout.fillWidth: true
                    textRole: "text"
                    valueRole: "value"
                    // Language names are shown in their own language, so they are not translated.
                    model: [
                        { text: qsTr("System language"), value: "" },
                        { text: "English", value: "en" },
                        { text: "Italiano", value: "it" },
                    ]
                    Component.onCompleted: currentIndex = indexOfValue(Localization.language)
                    onActivated: Localization.language = currentValue
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: Theme.gapS

                    Repeater {
                        model: Appearance.accentPresets

                        delegate: Rectangle {
                            required property color modelData

                            width: 22
                            height: 22
                            radius: 11
                            color: modelData
                            border.width: Qt.colorEqual(modelData, Appearance.accent) ? 2 : 0
                            border.color: Theme.text

                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -6
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Appearance.accent = parent.modelData
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            color: Theme.border
        }

        // Content
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.gapXl
            spacing: Theme.gapL

            RowLayout {
                Layout.fillWidth: true

                Label {
                    Layout.fillWidth: true
                    text: qsTr("All notes")
                    color: Theme.text
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.Bold
                }

                Button {
                    text: qsTr("New note")
                    highlighted: true
                    onClicked: page.openNote(qsTr("Untitled note"))
                }
            }

            GridView {
                id: grid

                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                cellWidth: 200
                cellHeight: 260
                model: page.fakeNotes

                delegate: Item {
                    id: cell

                    required property string modelData

                    width: grid.cellWidth
                    height: grid.cellHeight

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: Theme.gapS
                        radius: Theme.radiusCard
                        color: Theme.surface
                        border.color: Theme.border

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.gapM
                            spacing: Theme.gapS

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: Theme.radiusControl - 4
                                color: Theme.surfaceAlt
                            }

                            Label {
                                Layout.fillWidth: true
                                text: cell.modelData
                                color: Theme.text
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: page.openNote(cell.modelData)
                        }
                    }
                }
            }
        }
    }
}
