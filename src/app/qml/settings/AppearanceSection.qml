// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

// Settings › Appearance: theme, accent color, tablet mode, pen and touch.
ColumnLayout {
    id: section

    spacing: 32

    // Preset names, in the order of Appearance.accentPresets.
    readonly property var presetNames: [
        qsTr("Blue"), qsTr("Violet"), qsTr("Raspberry"), qsTr("Orange"),
        qsTr("Green"), qsTr("Teal"), qsTr("Sky"), qsTr("Graphite"),
    ]
    readonly property bool customAccent:
        !Appearance.accentPresets.some(c => Qt.colorEqual(c, Appearance.accent))

    component SectionTitle: Label {
        color: Theme.text
        font.pixelSize: Theme.fontSection
        font.weight: Font.DemiBold
    }

    Label {
        text: qsTr("Appearance")
        color: Theme.text
        font.pixelSize: 24
        font.weight: Font.Bold
        font.letterSpacing: -0.24
    }

    // Theme
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 14

        SectionTitle { text: qsTr("Theme") }

        GridLayout {
            Layout.fillWidth: true
            columns: 3
            columnSpacing: 14

            Repeater {
                model: [
                    { scheme: "light", mode: Appearance.Light, label: qsTr("Light") },
                    { scheme: "dark", mode: Appearance.Dark, label: qsTr("Dark") },
                    { scheme: "system", mode: Appearance.System, label: qsTr("Follow system") },
                ]

                delegate: ThemeCard {
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.preferredWidth: 1 // equal columns
                    scheme: modelData.scheme
                    text: modelData.label
                    checked: Appearance.mode === modelData.mode
                    onClicked: Appearance.mode = modelData.mode
                }
            }
        }
    }

    // Accent color
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 14

        ColumnLayout {
            spacing: Theme.gapXs

            SectionTitle { text: qsTr("Accent color") }

            Label {
                Layout.fillWidth: true
                text: qsTr("Used for buttons, the active tool and selections. Backgrounds stay neutral.")
                wrapMode: Text.Wrap
                color: Theme.textMuted
                font.pixelSize: 13
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: Theme.gapXs

            Repeater {
                model: Appearance.accentPresets

                delegate: AccentSwatch {
                    required property color modelData
                    required property int index

                    color: modelData
                    text: section.presetNames[index] ?? ""
                    checked: Qt.colorEqual(modelData, Appearance.accent)
                    onClicked: Appearance.accent = modelData
                }
            }

            // Custom color
            AbstractButton {
                id: customButton

                implicitHeight: 48
                leftPadding: 10
                rightPadding: 14
                hoverEnabled: true
                text: qsTr("Custom")
                Accessible.role: Accessible.RadioButton
                Accessible.checked: section.customAccent
                onClicked: colorDialog.open()

                contentItem: Row {
                    spacing: 10

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 30
                        height: 30
                        radius: 15
                        color: Appearance.accent
                    }

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: customButton.text
                        color: Theme.textSecondary
                        font.pixelSize: 13
                        font.weight: Font.Medium
                    }

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: Appearance.accent.toString().toUpperCase()
                        color: Theme.textMuted
                        font.family: "monospace"
                        font.pixelSize: 12
                    }
                }

                background: Rectangle {
                    radius: 24
                    color: customButton.hovered ? Theme.hover : "transparent"
                    border.width: section.customAccent ? 2 : 1
                    border.color: section.customAccent ? Theme.accentInk : Theme.borderStrong

                    FocusRing { visible: customButton.visualFocus }
                }
            }
        }

        ColorDialog {
            id: colorDialog

            title: qsTr("Accent color")
            selectedColor: Appearance.accent
            onAccepted: Appearance.accent = selectedColor
        }

        AccentPreview { Layout.fillWidth: true }
    }

    // Tablet mode
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 14

        SectionTitle { text: qsTr("Tablet mode") }

        OptionGroup {
            Layout.fillWidth: true

            Repeater {
                model: [
                    { mode: Preferences.Automatic, label: qsTr("Automatic"),
                      description: qsTr("Turns on when the laptop is folded or the keyboard is detached") },
                    { mode: Preferences.Always, label: qsTr("Always on"),
                      description: qsTr("Large controls even with keyboard and mouse") },
                    { mode: Preferences.Never, label: qsTr("Never"),
                      description: qsTr("Always use the desktop layout") },
                ]

                delegate: OptionRow {
                    required property var modelData
                    required property int index

                    radio: true
                    text: modelData.label
                    description: modelData.description
                    checked: Preferences.tabletMode === modelData.mode
                    showDivider: index < 2
                    onClicked: Preferences.tabletMode = modelData.mode
                }
            }
        }
    }

    // Pen and touch
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 14

        SectionTitle { text: qsTr("Pen and touch") }

        OptionGroup {
            Layout.fillWidth: true

            OptionRow {
                text: qsTr("Only the stylus draws")
                description: qsTr("Fingers scroll and zoom, with no accidental strokes")
                checked: Preferences.stylusOnly
                onClicked: Preferences.stylusOnly = checked
            }

            OptionRow {
                text: qsTr("Stylus button as eraser")
                description: qsTr("Hold the side button to erase")
                checked: Preferences.barrelButtonErases
                onClicked: Preferences.barrelButtonErases = checked
            }

            OptionRow {
                text: qsTr("Two-finger tap to undo")
                description: qsTr("Three fingers to redo")
                checked: Preferences.tapToUndo
                showDivider: false
                onClicked: Preferences.tapToUndo = checked
            }
        }
    }
}
