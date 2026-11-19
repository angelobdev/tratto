// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

// Text segmented control. `options` is a list of { value, label }.
Rectangle {
    id: control

    property var options: []
    property var value

    signal picked(var value)

    implicitWidth: row.implicitWidth + 6
    implicitHeight: row.implicitHeight + 6
    radius: Theme.radiusControl
    color: Theme.surfaceAlt

    Row {
        id: row

        anchors.centerIn: parent
        spacing: 2

        Repeater {
            model: control.options

            delegate: AbstractButton {
                id: segment

                required property var modelData
                readonly property bool current: modelData.value === control.value

                implicitHeight: 38
                leftPadding: 14
                rightPadding: 14
                hoverEnabled: true
                text: modelData.label
                checkable: true
                checked: current
                Accessible.role: Accessible.RadioButton
                onClicked: control.picked(modelData.value)

                contentItem: Label {
                    text: segment.text
                    verticalAlignment: Text.AlignVCenter
                    color: segment.current ? Theme.text : Theme.textSecondary
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }

                background: Rectangle {
                    radius: 8
                    color: segment.current ? Theme.surface : segment.hovered ? Theme.hover : "transparent"
                    border.width: segment.current && Theme.dark ? 1 : 0
                    border.color: Theme.border

                    FocusRing { visible: segment.visualFocus }
                }
            }
        }
    }
}
