// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

// Grid with as many equal columns as fit, each at least `minCellWidth` wide
// (like CSS `repeat(auto-fill, minmax(min, 1fr))`). Children bind
// `width: parent.cellWidth`.
Grid {
    id: grid

    property real minCellWidth: 168

    columns: Math.max(1, Math.floor((width + columnSpacing) / (minCellWidth + columnSpacing)))
    readonly property real cellWidth: (width - (columns - 1) * columnSpacing) / columns
}
