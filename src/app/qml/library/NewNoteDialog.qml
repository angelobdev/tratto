// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

// New note: title, group, paper (pattern, color, format, orientation) with a live
// preview. Importing a PDF or image arrives in M4.
Popup {
    id: dialog

    // Emitted with the created note's id.
    signal created(string noteId)

    property string preselectedGroup: ""

    // Current choice, initialized from the saved defaults on open.
    property int pattern: 1
    property real spacingMm: 6
    property color paperColor: "#FFFFFF"
    property int format: 0      // 0 A4, 1 Letter, 2 Continuous
    property int orientation: 0 // 0 portrait, 1 landscape
    property string groupId: ""

    readonly property var backgrounds: [
        { pattern: PaperThumbnail.Blank, spacingMm: 0, label: qsTr("Blank"), detail: qsTr("No lines") },
        { pattern: PaperThumbnail.Ruled, spacingMm: 6, label: qsTr("Ruled"), detail: qsTr("%1 mm").arg(6) },
        { pattern: PaperThumbnail.Ruled, spacingMm: 8, label: qsTr("Ruled"), detail: qsTr("%1 mm").arg(8) },
        { pattern: PaperThumbnail.Grid, spacingMm: 5, label: qsTr("Grid"), detail: qsTr("%1 mm").arg(5) },
        { pattern: PaperThumbnail.Grid, spacingMm: 10, label: qsTr("Grid"), detail: qsTr("%1 mm").arg(10) },
        { pattern: PaperThumbnail.Dots, spacingMm: 5, label: qsTr("Dotted"), detail: qsTr("%1 mm").arg(5) },
    ]
    readonly property var paperColors: [
        // `summary` is a whole phrase so each language can agree it with "paper".
        { color: "#FFFFFF", label: qsTr("White"), summary: qsTr("white paper") },
        { color: "#FBF6EA", label: qsTr("Ivory"), summary: qsTr("ivory paper") },
        { color: "#F1F3F5", label: qsTr("Pearl gray"), summary: qsTr("pearl gray paper") },
        { color: "#EEF3EA", label: qsTr("Sage"), summary: qsTr("sage paper") },
        { color: "#2B2D31", label: qsTr("Slate"), summary: qsTr("slate paper") },
    ]
    readonly property var currentBackground:
        backgrounds.find(b => b.pattern === pattern && b.spacingMm === spacingMm) ?? backgrounds[0]
    readonly property bool customPaper: !paperColors.some(p => Qt.colorEqual(p.color, paperColor))

    function paperChoice() {
        return { pattern: pattern, spacingMm: spacingMm, color: paperColor.toString(),
                 format: format, orientation: orientation }
    }

    function create() {
        const paper = paperChoice()
        if (useAsDefault.checked)
            Preferences.newNotePaper = paper
        const id = Library.createNote(titleField.text.trim(), groupId, paper.pattern, paper.spacingMm,
                                      paper.color, paper.format, paper.orientation)
        close()
        created(id)
    }

    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(1060, (parent ? parent.width : 1060) - 48)
    height: Math.min(implicitHeight, (parent ? parent.height : 800) - 48)
    padding: 0

    onOpened: {
        const d = Preferences.newNotePaper
        pattern = d.pattern
        spacingMm = d.spacingMm
        paperColor = d.color
        format = d.format
        orientation = d.orientation
        groupId = preselectedGroup
        titleField.text = ""
        useAsDefault.checked = false
        titleField.forceActiveFocus()
    }

    background: Rectangle {
        radius: Theme.radiusDialog
        color: Theme.surface
        border.color: Theme.border
    }

    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, Theme.dark ? 0.5 : 0.3) }

    component FieldLabel: Label {
        color: Theme.textSecondary
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }

    contentItem: ColumnLayout {
        spacing: 0

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 28
            Layout.rightMargin: 20
            Layout.topMargin: 18
            Layout.bottomMargin: 18

            Label {
                Layout.fillWidth: true
                text: qsTr("New note")
                color: Theme.text
                font.pixelSize: Theme.fontDialogTitle
                font.weight: Font.Bold
            }

            IconButton {
                size: Theme.controlHeight
                radius: Theme.radiusControl
                iconSize: 20
                iconName: "x"
                text: qsTr("Close")
                onClicked: dialog.close()
            }
        }

        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

        // Body: options on the left, preview on the right (below on narrow windows).
        ScrollView {
            id: bodyScroll

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: body.implicitHeight
            contentWidth: availableWidth

            GridLayout {
                id: body

                width: bodyScroll.availableWidth
                columns: width > 820 ? 2 : 1
                columnSpacing: 0
                rowSpacing: 0

                // Options
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    Layout.margins: 28
                    Layout.topMargin: 24
                    spacing: Theme.gapXl

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: Theme.gapL
                        rowSpacing: 6

                        FieldLabel { text: qsTr("Title") }
                        FieldLabel { text: qsTr("Group") }

                        TextField {
                            id: titleField

                            Layout.fillWidth: true
                            Layout.preferredWidth: 2
                            implicitHeight: Theme.controlHeight
                            placeholderText: qsTr("Untitled")
                            font.pixelSize: 15
                            color: Theme.text
                            placeholderTextColor: Theme.textMuted
                            Accessible.name: qsTr("Title")
                            onAccepted: dialog.create()

                            background: Rectangle {
                                radius: Theme.radiusControl
                                color: Theme.surface
                                border.width: titleField.activeFocus ? 2 : 1
                                border.color: titleField.activeFocus ? Theme.accentInk : Theme.borderStrong
                            }
                        }

                        CompactComboBox {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            implicitHeight: Theme.controlHeight
                            font.pixelSize: 15
                            Accessible.name: qsTr("Group")
                            model: [{ value: "", text: qsTr("None (standalone note)") }]
                                .concat(Library.groups.map(g => ({ value: g.id, text: g.name })))
                                .concat([{ value: "+new", text: qsTr("+ New group…") }])
                            value: dialog.groupId
                            onPicked: value => {
                                if (value === "+new")
                                    groupDialog.open()
                                else
                                    dialog.groupId = value
                            }
                        }
                    }

                    // Note type
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 12

                        Repeater {
                            model: [
                                { kind: "blank", icon: "file-plus", label: qsTr("Blank document"),
                                  detail: qsTr("Choose background, color and format") },
                                { kind: "import", icon: "download", label: qsTr("Import PDF or image"),
                                  detail: qsTr("Annotate an existing file") },
                            ]

                            delegate: AbstractButton {
                                id: typeCard

                                required property var modelData
                                readonly property bool current: modelData.kind === "blank"

                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                padding: Theme.gapL
                                checkable: true
                                checked: current
                                enabled: modelData.kind === "blank" // import arrives in M4
                                opacity: enabled ? 1 : 0.5
                                Accessible.role: Accessible.RadioButton
                                Accessible.name: modelData.label

                                contentItem: RowLayout {
                                    spacing: 14

                                    Rectangle {
                                        implicitWidth: 44
                                        implicitHeight: 44
                                        radius: Theme.radiusControl
                                        color: Theme.surface
                                        border.color: Theme.border

                                        Icon {
                                            anchors.centerIn: parent
                                            name: typeCard.modelData.icon
                                            size: 22
                                            color: Theme.accentInk
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        Label {
                                            Layout.fillWidth: true
                                            text: typeCard.modelData.label
                                            elide: Text.ElideRight
                                            color: Theme.text
                                            font.pixelSize: 15
                                            font.weight: Font.DemiBold
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: typeCard.modelData.detail
                                            elide: Text.ElideRight
                                            color: Theme.textMuted
                                            font.pixelSize: 13
                                        }
                                    }
                                }

                                background: Rectangle {
                                    radius: Theme.radiusCard
                                    color: typeCard.current ? Theme.accentSoft : Theme.surface
                                    border.width: typeCard.current ? 2 : 1
                                    border.color: typeCard.current ? Theme.accentInk : Theme.border
                                }
                            }
                        }
                    }

                    // Background
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        FieldLabel { text: qsTr("Background") }

                        ResponsiveGrid {
                            Layout.fillWidth: true
                            minCellWidth: 92
                            columnSpacing: 12
                            rowSpacing: 12

                            Repeater {
                                model: dialog.backgrounds

                                delegate: AbstractButton {
                                    id: bgChoice

                                    required property var modelData
                                    readonly property bool current: dialog.pattern === modelData.pattern
                                                                    && dialog.spacingMm === modelData.spacingMm

                                    width: parent.cellWidth
                                    checkable: true
                                    checked: current
                                    Accessible.role: Accessible.RadioButton
                                    Accessible.name: modelData.label + " " + modelData.detail
                                    onClicked: {
                                        dialog.pattern = modelData.pattern
                                        dialog.spacingMm = modelData.spacingMm
                                    }

                                    contentItem: ColumnLayout {
                                        spacing: Theme.gapS

                                        Item {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: width * 270 / 210

                                            Rectangle {
                                                visible: bgChoice.current || bgChoice.visualFocus
                                                anchors.fill: parent
                                                anchors.margins: -4
                                                radius: 12
                                                color: "transparent"
                                                border.width: 2
                                                border.color: Theme.accentInk
                                            }

                                            // A zoomed-in part of the page (below the ruled margin),
                                            // so the pattern is readable at this size.
                                            PaperThumbnail {
                                                anchors.fill: parent
                                                radius: 8
                                                viewport: Qt.rect(0, 70, 150, 150 * 270 / 210)
                                                pattern: bgChoice.modelData.pattern
                                                spacingMm: bgChoice.modelData.spacingMm
                                                paperColor: dialog.paperColor
                                            }

                                            Rectangle {
                                                anchors.fill: parent
                                                radius: 8
                                                color: "transparent"
                                                border.color: Theme.borderStrong
                                            }
                                        }

                                        Label {
                                            Layout.alignment: Qt.AlignHCenter
                                            text: bgChoice.modelData.label
                                            color: Theme.text
                                            font.pixelSize: 13
                                            font.weight: Font.DemiBold
                                        }

                                        Label {
                                            Layout.alignment: Qt.AlignHCenter
                                            text: bgChoice.modelData.detail
                                            color: Theme.textMuted
                                            font.pixelSize: Theme.fontLabel
                                        }
                                    }

                                    background: Item {}
                                }
                            }
                        }
                    }

                    // Paper color
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        FieldLabel { text: qsTr("Paper color") }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 6

                            Repeater {
                                model: dialog.paperColors

                                delegate: AbstractButton {
                                    id: paperChoice

                                    required property var modelData
                                    readonly property bool current: Qt.colorEqual(dialog.paperColor, modelData.color)

                                    implicitWidth: 44
                                    implicitHeight: 44
                                    checkable: true
                                    checked: current
                                    Accessible.role: Accessible.RadioButton
                                    Accessible.name: modelData.label
                                    ToolTip.visible: hovered
                                    ToolTip.text: modelData.label
                                    ToolTip.delay: 600
                                    hoverEnabled: true
                                    onClicked: dialog.paperColor = modelData.color

                                    contentItem: Item {}
                                    background: Item {
                                        Rectangle {
                                            visible: paperChoice.current || paperChoice.visualFocus
                                            anchors.centerIn: parent
                                            width: 38
                                            height: 38
                                            radius: 19
                                            color: "transparent"
                                            border.width: 2
                                            border.color: Theme.accentInk
                                        }

                                        Rectangle {
                                            anchors.centerIn: parent
                                            width: 30
                                            height: 30
                                            radius: 15
                                            color: paperChoice.modelData.color
                                            border.color: Theme.dark ? Qt.rgba(1, 1, 1, 0.25)
                                                                     : Qt.rgba(0.094, 0.094, 0.106, 0.18)
                                        }
                                    }
                                }
                            }

                            AbstractButton {
                                id: customPaperButton

                                implicitHeight: 44
                                leftPadding: 8
                                rightPadding: 12
                                hoverEnabled: true
                                text: qsTr("Custom")
                                Accessible.role: Accessible.RadioButton
                                Accessible.checked: dialog.customPaper
                                onClicked: paperColorDialog.open()

                                contentItem: Row {
                                    spacing: Theme.gapS

                                    Rectangle {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 28
                                        height: 28
                                        radius: 14
                                        color: dialog.customPaper ? dialog.paperColor : "transparent"
                                        border.color: Theme.borderStrong
                                    }

                                    Label {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: customPaperButton.text
                                        color: Theme.textSecondary
                                        font.pixelSize: 13
                                        font.weight: Font.Medium
                                    }
                                }

                                background: Rectangle {
                                    radius: 22
                                    color: customPaperButton.hovered ? Theme.hover : "transparent"
                                    border.width: dialog.customPaper ? 2 : 1
                                    border.color: dialog.customPaper ? Theme.accentInk : Theme.borderStrong
                                }
                            }
                        }
                    }

                    // Format and orientation
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gapXl

                        ColumnLayout {
                            spacing: 10

                            FieldLabel { text: qsTr("Format") }

                            SegmentedChoice {
                                value: dialog.format
                                options: [
                                    { value: 0, label: "A4" },
                                    { value: 1, label: "Letter" },
                                    { value: 2, label: qsTr("Continuous") },
                                ]
                                onPicked: value => dialog.format = value
                            }
                        }

                        ColumnLayout {
                            visible: dialog.format !== 2
                            spacing: 10

                            FieldLabel { text: qsTr("Orientation") }

                            SegmentedChoice {
                                value: dialog.orientation
                                options: [
                                    { value: 0, label: qsTr("Portrait") },
                                    { value: 1, label: qsTr("Landscape") },
                                ]
                                onPicked: value => dialog.orientation = value
                            }
                        }
                    }
                }

                // Preview
                Rectangle {
                    Layout.fillWidth: body.columns === 1
                    Layout.fillHeight: true
                    Layout.preferredWidth: 340
                    implicitHeight: preview.implicitHeight + 56
                    color: Theme.background

                    Rectangle {
                        visible: body.columns === 2
                        width: 1
                        height: parent.height
                        color: Theme.border
                    }

                    ColumnLayout {
                        id: preview

                        anchors.fill: parent
                        anchors.margins: 28
                        spacing: Theme.gapL

                        Label {
                            text: qsTr("Preview").toUpperCase()
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontLabel
                            font.weight: Font.DemiBold
                            font.letterSpacing: Theme.fontLabel * 0.06
                        }

                        PaperThumbnail {
                            readonly property bool landscape: dialog.format !== 2 && dialog.orientation === 1
                            readonly property real aspect: dialog.format === 1 ? 792 / 612 : 841.89 / 595.28

                            Layout.alignment: Qt.AlignHCenter
                            implicitWidth: landscape ? 280 * Math.min(aspect, 330 / 280) : 280
                            implicitHeight: landscape ? implicitWidth / aspect : 280 * aspect
                            radius: 4
                            pattern: dialog.pattern
                            spacingMm: dialog.spacingMm
                            paperColor: dialog.paperColor

                            Rectangle {
                                anchors.fill: parent
                                radius: 4
                                color: "transparent"
                                border.color: Theme.border
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            color: Theme.textSecondary
                            font.pixelSize: 13
                            text: {
                                const bg = dialog.currentBackground
                                const paper = dialog.paperColors.find(p => Qt.colorEqual(p.color, dialog.paperColor))
                                const parts = [bg.spacingMm > 0 ? bg.label + " " + bg.detail : bg.label,
                                               paper ? paper.summary : qsTr("custom paper"),
                                               ["A4", "Letter", qsTr("Continuous")][dialog.format]]
                                if (dialog.format !== 2)
                                    parts.push(dialog.orientation === 0 ? qsTr("portrait") : qsTr("landscape"))
                                return parts.join(" · ")
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

        // Footer
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 28
            Layout.rightMargin: 28
            Layout.topMargin: Theme.gapL
            Layout.bottomMargin: Theme.gapL
            spacing: Theme.gapM

            CheckOption {
                id: useAsDefault

                Layout.fillWidth: true
                text: qsTr("Use as default for new notes")
                leftPadding: 0
            }

            AppButton {
                text: qsTr("Cancel")
                onClicked: dialog.close()
            }

            AppButton {
                primary: true
                text: qsTr("Create note")
                onClicked: dialog.create()
            }
        }
    }

    ColorDialog {
        id: paperColorDialog

        title: qsTr("Paper color")
        selectedColor: dialog.paperColor
        onAccepted: dialog.paperColor = selectedColor
    }

    GroupDialog {
        id: groupDialog

        onSubmitted: (name, color) => dialog.groupId = Library.createGroup(name, color)
    }
}
