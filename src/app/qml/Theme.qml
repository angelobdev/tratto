// SPDX-License-Identifier: GPL-3.0-or-later
pragma Singleton

import QtQuick

// Design tokens from docs/DESIGN.md. Colors that change at runtime
// (light/dark, accent) come from the C++ `Appearance` singleton.
QtObject {
    readonly property bool dark: Appearance.dark

    // Interface colors
    readonly property color background: dark ? "#18181B" : "#F4F4F5"
    readonly property color canvas: dark ? "#111113" : "#F4F4F5"
    readonly property color sidebar: dark ? "#1C1C1F" : "#EDEDEF"
    readonly property color surface: dark ? "#202024" : "#FFFFFF"
    readonly property color surfaceAlt: dark ? "#2A2A2F" : "#F0F0F2"
    readonly property color border: dark ? "#2E2E33" : "#E4E4E7"
    readonly property color borderStrong: dark ? "#3F3F46" : "#D4D4D8"
    readonly property color text: dark ? "#F4F4F5" : "#18181B"
    readonly property color textSecondary: dark ? "#D4D4D8" : "#3F3F46"
    readonly property color textMuted: dark ? "#A1A1AA" : "#5B5B63"
    readonly property color danger: dark ? "#FFA8A8" : "#C92A2A"

    // Accent (user-chosen)
    readonly property color accent: Appearance.accent
    readonly property color onAccent: Appearance.onAccent
    readonly property color accentSoft: Appearance.accentSoft
    readonly property color accentInk: Appearance.accentInk

    // Sizes
    readonly property int topBarHeight: 56
    readonly property int sidebarWidth: 248
    readonly property int controlHeight: 44
    readonly property int radiusControl: 10
    readonly property int radiusCard: 12
    readonly property int radiusDialog: 16

    // Spacing (4 px grid)
    readonly property int gapXs: 4
    readonly property int gapS: 8
    readonly property int gapM: 12
    readonly property int gapL: 16
    readonly property int gapXl: 24

    // Typography
    readonly property string fontFamily: "Inter"
    readonly property int fontBody: 14
    readonly property int fontLabel: 12
    readonly property int fontTitle: 26
    readonly property int fontDialogTitle: 19
}
