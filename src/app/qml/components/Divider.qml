// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

// Thin vertical separator for toolbars.
Rectangle {
    property int length: 28

    Layout.leftMargin: 6
    Layout.rightMargin: 6
    implicitWidth: 1
    implicitHeight: length
    color: Theme.border
}
