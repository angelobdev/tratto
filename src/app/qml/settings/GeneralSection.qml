// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Settings › General: interface language.
ColumnLayout {
    spacing: 32

    Label {
        text: qsTr("General")
        color: Theme.text
        font.pixelSize: 24
        font.weight: Font.Bold
        font.letterSpacing: -0.24
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 14

        Label {
            text: qsTr("Language")
            color: Theme.text
            font.pixelSize: Theme.fontSection
            font.weight: Font.DemiBold
        }

        OptionGroup {
            Layout.fillWidth: true

            Repeater {
                // Language names are shown in their own language, so they are not translated.
                model: [
                    { code: "", label: qsTr("System language"),
                      description: qsTr("Follows the language of your system") },
                    { code: "en", label: "English", description: "" },
                    { code: "it", label: "Italiano", description: "" },
                ]

                delegate: OptionRow {
                    required property var modelData
                    required property int index

                    radio: true
                    text: modelData.label
                    description: modelData.description
                    checked: Localization.language === modelData.code
                    showDivider: index < 2
                    onClicked: Localization.language = modelData.code
                }
            }
        }
    }
}
