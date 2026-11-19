// SPDX-License-Identifier: GPL-3.0-or-later
// Lucide icons (https://lucide.dev, ISC license, see LUCIDE-LICENSE), as SVG path
// data on a 24 x 24 grid. Shapes such as <rect> and <circle> are converted to paths.
// Add icons here by name; Icon.qml draws them.
pragma Singleton

import QtQml

QtObject {
    readonly property var paths: ({
        "pen": "M16.5 3.5a2.1 2.1 0 0 1 3 3L7 19l-4 1 1-4Z",
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
        "undo": "M9 14 4 9l5-5 M4 9h10.5a5.5 5.5 0 0 1 0 11H11",
        "redo": "M15 14l5-5-5-5 M20 9H9.5a5.5 5.5 0 0 0 0 11H13",
        "check": "M20 6 9 17l-5-5",
        "tablet": "M6 2h12a2 2 0 0 1 2 2v16a2 2 0 0 1 -2 2h-12a2 2 0 0 1 -2 -2v-16a2 2 0 0 1 2 -2Z M12 18h.01",
        "panel-right": "M5 3h14a2 2 0 0 1 2 2v14a2 2 0 0 1 -2 2h-14a2 2 0 0 1 -2 -2v-14a2 2 0 0 1 2 -2Z M15 3v18",
        "upload": "M12 3v12 M7 8l5-5 5 5 M20 15v4a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2v-4",
        "ellipsis": "M3.4 12a1.6 1.6 0 1 0 3.2 0a1.6 1.6 0 1 0 -3.2 0Z M10.4 12a1.6 1.6 0 1 0 3.2 0a1.6 1.6 0 1 0 -3.2 0Z M17.4 12a1.6 1.6 0 1 0 3.2 0a1.6 1.6 0 1 0 -3.2 0Z",
        "highlighter": "M9 11l-6 6v3h9l3-3 M22 12l-4.6 4.6a2 2 0 0 1-2.8 0l-5.2-5.2a2 2 0 0 1 0-2.8L14 4",
        "eraser": "M7 21l-4.3-4.3c-1-1-1-2.5 0-3.4l9.6-9.6c1-1 2.5-1 3.4 0l5.6 5.6c1 1 1 2.5 0 3.4L13 21 M22 21H7 M5 11l9 9",
        "type": "M4 7V4h16v3 M9 20h6 M12 4v16",
        "lasso": "M7 22a5 5 0 0 1-2-4 M3.3 14A6.8 6.8 0 0 1 2 10c0-4.4 4.5-8 10-8s10 3.6 10 8-4.5 8-10 8a12 12 0 0 1-5-1 M3 16a2 2 0 1 0 4 0a2 2 0 1 0 -4 0Z",
        "image": "M5 3h14a2 2 0 0 1 2 2v14a2 2 0 0 1 -2 2h-14a2 2 0 0 1 -2 -2v-14a2 2 0 0 1 2 -2Z M7 9a2 2 0 1 0 4 0a2 2 0 1 0 -4 0Z M21 15l-3.1-3.1a2 2 0 0 0-2.8 0L6 21",
        "minus": "M5 12h14",
    })
}
