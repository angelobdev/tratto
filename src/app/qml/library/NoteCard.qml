// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Note card: page thumbnail, title, group and last modification time.
AbstractButton {
    id: control

    required property var note // see FakeLibrary.notes
    readonly property var group: FakeLibrary.group(note.group)

    hoverEnabled: true
    padding: 0
    Accessible.name: note.title

    contentItem: ColumnLayout {
        spacing: 10

        // Thumbnail, A4 proportions.
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: width * 297 / 210

            PaperThumbnail {
                anchors.fill: parent
                radius: 10
                pattern: control.note.paper.pattern
                spacingMm: control.note.paper.spacingMm
                paperColor: control.note.paper.color
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
                text: control.note.title
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
                    color: control.group ? control.group.color : "transparent"
                    border.width: control.group ? 0 : 1.5
                    border.color: Theme.standaloneMarker
                }

                Label {
                    Layout.fillWidth: true
                    text: (control.group ? control.group.name : qsTr("Standalone note"))
                          + " · " + Format.noteTimestamp(control.note.modified)
                    elide: Text.ElideRight
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontLabel
                }
            }
        }
    }
}
