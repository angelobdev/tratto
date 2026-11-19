// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Settings: section list on the left, the selected section on the right.
// Only General and Appearance exist so far; the other sections are listed but disabled.
Page {
    id: page

    property string section: "appearance"

    signal back()

    background: Rectangle { color: Theme.background }

    Keys.onEscapePressed: page.back()

    component SectionItem: AbstractButton {
        id: item

        property string key

        Layout.fillWidth: true
        implicitHeight: 40
        leftPadding: 12
        rightPadding: 12
        hoverEnabled: true
        opacity: enabled ? 1 : 0.45
        Accessible.role: Accessible.PageTab
        Accessible.selected: page.section === key
        onClicked: page.section = key

        readonly property bool current: page.section === key

        contentItem: Label {
            text: item.text
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            color: item.current ? Theme.accentInk : Theme.textSecondary
            font.pixelSize: 14
            font.weight: item.current ? Font.DemiBold : Font.Medium
        }

        background: Rectangle {
            radius: 8
            color: item.current ? Theme.accentSoft
                 : item.down ? Theme.pressed
                 : item.hovered ? Theme.hover : "transparent"

            FocusRing { visible: item.visualFocus }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Section list
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: Theme.sidebarWidth
            color: Theme.sidebar

            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: Theme.border
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 16
                anchors.bottomMargin: 16
                anchors.leftMargin: 14
                anchors.rightMargin: 15
                spacing: Theme.gapXs

                AbstractButton {
                    id: backButton

                    implicitHeight: Theme.controlHeight
                    leftPadding: 4
                    rightPadding: 10
                    hoverEnabled: true
                    text: qsTr("Library")
                    Accessible.name: qsTr("Back to the library")
                    onClicked: page.back()

                    contentItem: RowLayout {
                        spacing: Theme.gapXs

                        Icon {
                            name: "chevron-left"
                            size: 20
                            strokeWidth: 2
                        }

                        Label {
                            text: backButton.text
                            color: Theme.textSecondary
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }
                    }

                    background: Rectangle {
                        radius: Theme.radiusControl
                        color: backButton.down ? Theme.pressed : backButton.hovered ? Theme.hover : "transparent"

                        FocusRing { visible: backButton.visualFocus }
                    }
                }

                Label {
                    Layout.topMargin: 12
                    Layout.bottomMargin: 14
                    Layout.leftMargin: 10
                    text: qsTr("Settings")
                    color: Theme.text
                    font.pixelSize: 22
                    font.weight: Font.Bold
                    font.letterSpacing: -0.22
                }

                SectionItem { key: "general"; text: qsTr("General") }
                SectionItem { key: "appearance"; text: qsTr("Appearance") }
                SectionItem { key: "input"; text: qsTr("Pen and input"); enabled: false }
                SectionItem { key: "newNotes"; text: qsTr("New notes"); enabled: false }
                SectionItem { key: "export"; text: qsTr("Export"); enabled: false }
                SectionItem { key: "storage"; text: qsTr("Storage and backup"); enabled: false }
                SectionItem { key: "about"; text: qsTr("About"); enabled: false }

                Item { Layout.fillHeight: true }
            }
        }

        // Selected section
        ScrollView {
            id: scroll

            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth

            Item {
                width: scroll.availableWidth
                implicitHeight: loader.implicitHeight + 36 + 48

                Loader {
                    id: loader

                    x: 48
                    y: 36
                    width: Math.min(760, parent.width - 96)
                    sourceComponent: page.section === "general" ? generalSection : appearanceSection
                }
            }
        }
    }

    Component { id: generalSection; GeneralSection {} }
    Component { id: appearanceSection; AppearanceSection {} }
}
