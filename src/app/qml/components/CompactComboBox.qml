// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

// 38 px combo box for toolbars. `model` is a list of { value, text };
// set `value` and handle `picked(value)`.
ComboBox {
    id: control

    property var value

    signal picked(var value)

    implicitHeight: 38
    textRole: "text"
    valueRole: "value"
    font.pixelSize: 13
    leftPadding: 10
    rightPadding: 30

    // `count` makes this re-evaluate once the model is loaded.
    currentIndex: count > 0 ? indexOfValue(value) : -1
    onActivated: control.picked(control.valueAt(currentIndex))

    indicator: Icon {
        x: control.width - width - 10
        anchors.verticalCenter: parent.verticalCenter
        name: "chevron-down"
        size: 14
        strokeWidth: 2
    }

    contentItem: Label {
        text: control.displayText
        verticalAlignment: Text.AlignVCenter
        color: Theme.text
        font: control.font
    }

    background: Rectangle {
        implicitWidth: 72
        radius: 8
        color: control.hovered ? Theme.surfaceAlt : Theme.surface
        border.color: Theme.borderStrong

        FocusRing { visible: control.visualFocus }
    }
}
