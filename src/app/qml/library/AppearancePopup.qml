// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// TEMPORARY: theme, accent and language controls until the Settings dialog
// from the design replaces this popup.
Popup {
    id: popup

    modal: true
    padding: Theme.gapXl
    width: 320

    background: Rectangle {
        radius: Theme.radiusDialog
        color: Theme.surface
        border.color: Theme.border
    }

    contentItem: ColumnLayout {
        spacing: Theme.gapM

        Label {
            text: qsTr("Appearance")
            color: Theme.text
            font.pixelSize: Theme.fontDialogTitle
            font.weight: Font.Bold
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

                    width: 26
                    height: 26
                    radius: 13
                    color: modelData
                    border.width: Qt.colorEqual(modelData, Appearance.accent) ? 3 : 0
                    border.color: Theme.surface

                    Rectangle {
                        visible: Qt.colorEqual(parent.modelData, Appearance.accent)
                        anchors.fill: parent
                        anchors.margins: -2
                        radius: width / 2
                        color: "transparent"
                        border.width: 2
                        border.color: parent.modelData
                    }

                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -4
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Appearance.accent = parent.modelData
                    }
                }
            }
        }
    }
}
