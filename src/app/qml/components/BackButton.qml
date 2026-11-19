// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// "‹ Library" navigation button.
AbstractButton {
    id: control

    implicitHeight: Theme.controlHeight
    leftPadding: 6
    rightPadding: 12
    hoverEnabled: true

    contentItem: RowLayout {
        spacing: Theme.gapXs

        Icon {
            name: "chevron-left"
            size: 20
            strokeWidth: 2
        }

        Label {
            text: control.text
            color: Theme.textSecondary
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }
    }

    background: Rectangle {
        radius: Theme.radiusControl
        color: control.down ? Theme.pressed : control.hovered ? Theme.hover : "transparent"

        FocusRing { visible: control.visualFocus }
    }
}
