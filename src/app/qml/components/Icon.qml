// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Shapes

// Outline icon from Icons.qml, recolorable and crisp at any size.
Item {
    id: root

    property string name
    property color color: Theme.textSecondary
    property real size: 18
    property real strokeWidth: 1.8 // in the 24 x 24 icon grid, like the SVG

    implicitWidth: size
    implicitHeight: size

    Shape {
        width: 24
        height: 24
        scale: root.size / 24
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: root.color
            strokeWidth: root.strokeWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin

            PathSvg { path: Icons.paths[root.name] ?? "" }
        }
    }
}
