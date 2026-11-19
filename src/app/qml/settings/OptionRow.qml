// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One row of an OptionGroup: a radio choice (`radio: true`) or an on/off switch.
// Label and description on the left; radio dot on the left or switch on the right.
AbstractButton {
    id: row

    property bool radio: false
    property string description
    property bool showDivider: true

    Layout.fillWidth: true
    implicitHeight: Math.max(60, content.implicitHeight + 20)
    leftPadding: 18
    rightPadding: 18
    checkable: true
    hoverEnabled: true
    Accessible.role: radio ? Accessible.RadioButton : Accessible.CheckBox
    Accessible.name: text
    Accessible.description: description

    contentItem: RowLayout {
        id: content

        spacing: 14

        // Radio indicator
        Rectangle {
            visible: row.radio
            implicitWidth: 20
            implicitHeight: 20
            radius: 10
            color: "transparent"
            border.width: 2
            border.color: row.checked ? Theme.accentInk : "#71717A"

            Rectangle {
                anchors.centerIn: parent
                width: 10
                height: 10
                radius: 5
                color: row.checked ? Theme.accentInk : "transparent"
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: row.text
                wrapMode: Text.Wrap
                color: Theme.text
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }

            Label {
                visible: row.description !== ""
                Layout.fillWidth: true
                text: row.description
                wrapMode: Text.Wrap
                color: Theme.textMuted
                font.pixelSize: 13
            }
        }

        AppSwitch {
            visible: !row.radio
            checked: row.checked
        }
    }

    background: Item {
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 13
            color: row.down ? Theme.pressed : row.hovered ? Theme.hover : "transparent"
        }

        Rectangle {
            visible: row.showDivider
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.border
        }

        FocusRing {
            visible: row.visualFocus
            anchors.margins: 2
            radius: 12
        }
    }
}
