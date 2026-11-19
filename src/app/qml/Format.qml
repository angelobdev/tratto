// SPDX-License-Identifier: GPL-3.0-or-later
pragma Singleton

import QtQuick

// Localized formatting of counts and dates for the UI.
// Every function reads Localization.effectiveLanguage, so bindings that call
// them re-evaluate when the language changes.
QtObject {
    function noteCount(n) {
        Localization.effectiveLanguage
        return qsTr("%n note(s)", "", n)
    }

    function groupCount(n) {
        Localization.effectiveLanguage
        return qsTr("%n group(s)", "", n)
    }

    function daysAgo(date) {
        const start = d => new Date(d.getFullYear(), d.getMonth(), d.getDate())
        return Math.round((start(new Date()) - start(date)) / 86400000)
    }

    function shortDate(date) {
        return Qt.locale().toString(date, "d MMM")
    }

    function time(date) {
        return Qt.locale().toString(date, "HH:mm")
    }

    // "Today, 16:42" · "Yesterday, 21:03" · "17 Nov"
    function noteTimestamp(date) {
        Localization.effectiveLanguage
        switch (daysAgo(date)) {
        case 0: return qsTr("Today, %1").arg(time(date))
        case 1: return qsTr("Yesterday, %1").arg(time(date))
        default: return shortDate(date)
        }
    }

    // "updated today" · "updated yesterday" · "updated 17 Nov"
    function updated(date) {
        Localization.effectiveLanguage
        if (!date)
            return qsTr("empty")
        switch (daysAgo(date)) {
        case 0: return qsTr("updated today")
        case 1: return qsTr("updated yesterday")
        default: return qsTr("updated %1").arg(shortDate(date))
        }
    }
}
