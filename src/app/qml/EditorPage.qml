// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Editor skeleton: top bar and an empty A4 page. The canvas arrives in M1.
Page {
    id: page

    property string noteTitle

    signal back()

    background: Rectangle { color: Theme.canvas }

    header: Rectangle {
        implicitHeight: Theme.topBarHeight
        color: Theme.surface

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.border
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.gapS
            anchors.rightMargin: Theme.gapL
            spacing: Theme.gapS

            ToolButton {
                text: qsTr("Back")
                onClicked: page.back()
            }

            Label {
                Layout.fillWidth: true
                text: page.noteTitle
                color: Theme.text
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
        }
    }

    // A4 page, scaled to fit.
    Rectangle {
        readonly property real aspect: 595.28 / 841.89

        anchors.centerIn: parent
        height: parent.height - 2 * Theme.gapXl
        width: height * aspect
        color: "#FFFFFF"
        border.color: Theme.border

        Label {
            anchors.centerIn: parent
            text: qsTr("The canvas arrives in the next milestone.")
            color: "#5B5B63"
        }
    }
}
