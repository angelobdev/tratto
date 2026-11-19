// SPDX-License-Identifier: GPL-3.0-or-later
pragma Singleton

import QtQuick

// Placeholder library content until storage lands in M2.
// Reading Localization.effectiveLanguage makes the lists rebuild (and retranslate)
// when the language changes.
QtObject {
    id: library

    readonly property var groups: {
        Localization.effectiveLanguage
        return [
            { id: "university", name: qsTr("University"), color: "#AE3EC9",
              paper: { pattern: PaperThumbnail.Ruled, spacingMm: 6, color: "#FFFFFF" } },
            { id: "work", name: qsTr("Work"), color: "#E8590C",
              paper: { pattern: PaperThumbnail.Grid, spacingMm: 5, color: "#FFFFFF" } },
            { id: "home", name: qsTr("Home"), color: "#0CA678",
              paper: { pattern: PaperThumbnail.Blank, spacingMm: 0, color: "#FFFFFF" } },
        ]
    }

    // Sorted by modification time, newest first. `source` is "", "pdf" or "image".
    readonly property var notes: {
        Localization.effectiveLanguage
        return [
            note(qsTr("Calculus II — Double integrals"), "university", 0, 16, 42, PaperThumbnail.Ruled, 6, "#FFFFFF"),
            note(qsTr("Sprint 47 meeting"), "work", 0, 10, 15, PaperThumbnail.Grid, 5, "#FFFFFF"),
            note(qsTr("Membership form.pdf"), "home", 1, 21, 3, PaperThumbnail.Blank, 0, "#FFFFFF", "pdf"),
            note(qsTr("Rust notes — ownership"), "", 1, 18, 20, PaperThumbnail.Dots, 5, "#FFFFFF"),
            note(qsTr("Interface sketches"), "work", 2, 11, 30, PaperThumbnail.Blank, 0, "#FFFFFF"),
            note(qsTr("Physics — Thermodynamics"), "university", 4, 9, 50, PaperThumbnail.Ruled, 6, "#FFFFFF"),
            note(qsTr("Shopping list"), "", 5, 19, 5, PaperThumbnail.Ruled, 8, "#FBF6EA"),
            note(qsTr("Kitchen floor plan.jpg"), "home", 7, 14, 0, PaperThumbnail.Blank, 0, "#F1F3F5", "image"),
        ]
    }

    function note(title, group, daysAgo, hours, minutes, pattern, spacingMm, paperColor, source) {
        const modified = new Date()
        modified.setDate(modified.getDate() - daysAgo)
        modified.setHours(hours, minutes, 0, 0)
        return {
            id: title, title: title, group: group, modified: modified, source: source ?? "",
            paper: { pattern: pattern, spacingMm: spacingMm, color: paperColor },
        }
    }

    function group(id) {
        return groups.find(g => g.id === id) ?? null
    }

    function notesInGroup(id) {
        return notes.filter(n => n.group === id)
    }

    function lastModifiedInGroup(id) {
        const inGroup = notesInGroup(id)
        return inGroup.length > 0 ? inGroup[0].modified : null
    }
}
