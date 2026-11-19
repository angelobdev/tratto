// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

// 44 x 26 switch track with a white knob. Purely visual: the row that owns it
// handles input and accessibility.
Rectangle {
    id: track

    property bool checked: false

    implicitWidth: 44
    implicitHeight: 26
    radius: 13
    color: checked ? Theme.accent : (Theme.dark ? "#52525B" : "#A1A1AA")

    Behavior on color { ColorAnimation { duration: 120 } }

    Rectangle {
        width: 20
        height: 20
        radius: 10
        y: 3
        x: track.checked ? 21 : 3
        color: "#FFFFFF"

        Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
    }
}
