// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts

// Note card: page thumbnail, title, group and last modification time.
AbstractButton {
    id: control

    objectName: "noteCard"
    required property var note // an entry of Library.notes
    // Reading Library.groups re-evaluates this when groups are renamed or deleted.
    readonly property var group: {
        Library.groups
        return note.group !== "" ? Library.group(note.group) : ({})
    }
    readonly property bool hasGroup: group.id !== undefined

    signal contextMenuRequested()

    hoverEnabled: true
    onPressAndHold: contextMenuRequested() // touch and stylus

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: control.contextMenuRequested()
    }
    padding: 0
    Accessible.name: note.title

    contentItem: ColumnLayout {
        spacing: 10

        // Thumbnail, A4 proportions.
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: width * 297 / 210

            // Paper only, until the saved thumbnail (with the writing) is loaded.
            PaperThumbnail {
                anchors.fill: parent
                radius: 10
                visible: thumbnail.status !== Image.Ready
                pattern: control.note.paper.pattern
                spacingMm: control.note.paper.spacingMm
                paperColor: control.note.paper.color
            }

            Image {
                id: thumbnail

                anchors.fill: parent
                visible: false // drawn through the rounded mask below
                source: control.note.thumbnail
                asynchronous: true
                fillMode: Image.PreserveAspectCrop
                sourceSize.width: width * Screen.devicePixelRatio
            }

            MultiEffect {
                anchors.fill: parent
                visible: thumbnail.status === Image.Ready
                source: thumbnail
                maskEnabled: true
                maskSource: roundedMask
                maskThresholdMin: 0.5
                maskSpreadAtMin: 1.0
            }

            Rectangle {
                id: roundedMask

                anchors.fill: parent
                radius: 10
                visible: false
                layer.enabled: true
            }

            Rectangle {
                anchors.fill: parent
                radius: 10
                color: "transparent"
                border.color: control.hovered ? Theme.borderStrong : Theme.border

                FocusRing { visible: control.visualFocus }
            }

            // File-type badge for imported PDFs and images.
            Rectangle {
                visible: control.note.source !== ""
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 8
                implicitWidth: badgeLabel.implicitWidth + 12
                implicitHeight: badgeLabel.implicitHeight + 4
                radius: 5
                color: control.note.source === "pdf" ? Theme.pdfBadge : Theme.imageBadge

                Label {
                    id: badgeLabel

                    anchors.centerIn: parent
                    // File-type abbreviations, not translated.
                    text: control.note.source === "pdf" ? "PDF" : "IMG"
                    color: control.note.source === "pdf" ? Theme.pdfBadgeText : Theme.imageBadgeText
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    font.letterSpacing: 0.4
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                Layout.fillWidth: true
                text: control.note.title !== "" ? control.note.title : qsTr("Untitled note")
                elide: Text.ElideRight
                color: Theme.text
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                // Group color, or an empty square for standalone notes.
                Rectangle {
                    implicitWidth: 8
                    implicitHeight: 8
                    radius: 2
                    color: control.hasGroup ? control.group.color : "transparent"
                    border.width: control.hasGroup ? 0 : 1.5
                    border.color: Theme.standaloneMarker
                }

                Label {
                    Layout.fillWidth: true
                    text: (control.hasGroup ? control.group.name : qsTr("Standalone note"))
                          + " · " + (control.note.inTrash ? qsTr("Deleted %1").arg(Format.noteTimestamp(control.note.deleted))
                                                          : Format.noteTimestamp(control.note.modified))
                    elide: Text.ElideRight
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontLabel
                }
            }
        }
    }
}
