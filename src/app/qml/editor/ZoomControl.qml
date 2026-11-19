// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Floating pill: zoom out, zoom level, zoom in, page counter.
Rectangle {
    id: pill

    property real zoom: 1
    property bool canZoomIn: true
    property bool canZoomOut: true
    property int currentPage: 0
    property int pageCount: 1

    signal zoomInRequested()
    signal zoomOutRequested()

    implicitWidth: row.implicitWidth + 8
    implicitHeight: Theme.controlHeight
    radius: Theme.radiusCard
    color: Theme.surface
    border.color: Theme.border

    RowLayout {
        id: row

        anchors.centerIn: parent
        spacing: 2

        IconButton {
            size: 36
            radius: 8
            iconName: "minus"
            text: qsTr("Zoom out")
            enabled: pill.canZoomOut
            onClicked: pill.zoomOutRequested()
        }

        Label {
            Layout.minimumWidth: 48
            horizontalAlignment: Text.AlignHCenter
            text: Math.round(pill.zoom * 100) + "%"
            color: Theme.text
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        IconButton {
            size: 36
            radius: 8
            iconName: "plus"
            text: qsTr("Zoom in")
            enabled: pill.canZoomIn
            onClicked: pill.zoomInRequested()
        }

        Divider { length: 20 }

        Label {
            rightPadding: 10
            text: qsTr("Page %1 of %2").arg(pill.currentPage + 1).arg(pill.pageCount)
            color: Theme.textSecondary
            font.pixelSize: 13
        }
    }
}
