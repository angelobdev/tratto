// SPDX-License-Identifier: GPL-3.0-or-later
// Lucide icons (https://lucide.dev, ISC license, see LUCIDE-LICENSE), as SVG path
// data on a 24 x 24 grid. Shapes such as <rect> and <circle> are converted to paths.
// Add icons here by name; Icon.qml draws them.
pragma Singleton

import QtQml

QtObject {
    readonly property var paths: ({
        "pen-line": "M12 20h9 M16.5 3.5a2.1 2.1 0 0 1 3 3L7 19l-4 1 1-4Z",
        "notebook-text": "M6 2h12a2 2 0 0 1 2 2v16a2 2 0 0 1 -2 2h-12a2 2 0 0 1 -2 -2v-16a2 2 0 0 1 2 -2Z M8 7h8M8 11h8M8 15h5",
        "clock": "M3 12a9 9 0 1 0 18 0a9 9 0 1 0 -18 0Z M12 7v5l3 2",
        "file": "M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8Z M14 2v6h6",
        "plus": "M12 5v14M5 12h14",
        "trash": "M3 6h18 M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6 M8 6V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2",
        "sliders-vertical": "M4 21v-7M4 10V3M12 21v-9M12 8V3M20 21v-5M20 12V3M1 14h6M9 8h6M17 16h6",
        "search": "M4 11a7 7 0 1 0 14 0a7 7 0 1 0 -14 0Z M20 20l-3.5-3.5",
        "download": "M12 15V3 M7 10l5 5 5-5 M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4",
        "layout-grid": "M4 3h5a1 1 0 0 1 1 1v5a1 1 0 0 1 -1 1h-5a1 1 0 0 1 -1 -1v-5a1 1 0 0 1 1 -1Z M15 3h5a1 1 0 0 1 1 1v5a1 1 0 0 1 -1 1h-5a1 1 0 0 1 -1 -1v-5a1 1 0 0 1 1 -1Z M4 14h5a1 1 0 0 1 1 1v5a1 1 0 0 1 -1 1h-5a1 1 0 0 1 -1 -1v-5a1 1 0 0 1 1 -1Z M15 14h5a1 1 0 0 1 1 1v5a1 1 0 0 1 -1 1h-5a1 1 0 0 1 -1 -1v-5a1 1 0 0 1 1 -1Z",
        "list": "M8 6h13M8 12h13M8 18h13M3 6h.01M3 12h.01M3 18h.01",
        "chevron-down": "M6 9l6 6 6-6",
        "chevron-left": "M15 18l-6-6 6-6",
    })
}
