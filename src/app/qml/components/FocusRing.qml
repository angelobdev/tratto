// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

// Keyboard focus ring: 2 px of surface, then 2 px of accent (docs/DESIGN.md).
// Place it inside a control's background; it inherits the parent's radius.
Rectangle {
    anchors.fill: parent
    anchors.margins: -4
    radius: (parent.radius ?? 0) + 4
    color: "transparent"
    border.width: 2
    border.color: Theme.accentInk
}
