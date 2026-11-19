# Tratto — handwritten notes and PDF annotation (C++ + Qt Quick)

Native app for handwritten notes and for annotating PDFs and images.

- Linux first, on 2-in-1 laptops with a stylus.
- Windows and macOS later.

## Reference documents (read them before working)

- `docs/ARCHITECTURE.md`: **source of truth** for stack, layout, data model, autosave, milestones.
- `docs/DESIGN.md`: colors, sizes, typography.

If a decision changes, update `docs/ARCHITECTURE.md` in the same commit.

## Status

Current milestone: **M0** (CMake project, window, Library ↔ Editor navigation, theme + accent color, i18n wiring). Update this line whenever a milestone is completed.

## Commands

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/src/app/tratto
cmake --build build --target format    # clang-format
```

System dependencies (Qt ≥ 6.8, CMake ≥ 3.21, a C++23 compiler):

- Arch: `sudo pacman -S cmake ninja qt6-base qt6-declarative qt6-svg qt6-webengine qt6-tools` (Qt PDF ships in `qt6-webengine`).
- Fedora: `sudo dnf install cmake ninja-build qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtsvg-devel qt6-qtpdf-devel qt6-qttools-devel`.
- Ubuntu/Debian: `sudo apt install cmake ninja-build qt6-base-dev qt6-declarative-dev qt6-svg-dev qt6-pdf-dev qt6-tools-dev qt6-l10n-tools`.

## Rules

- **`core`, `render` and `storage` never link Qt Quick or Qt Qml.**
  - Logic goes there: geometry, eraser, selection, file format.
  - Its tests run headless (`QT_QPA_PLATFORM=offscreen`).
- **Coordinates are in pt, in page space.** Screen ↔ page conversion happens only in the canvas item.
- **Never block the GUI thread.**
  - I/O, PDF rasterization and tile rendering run on other threads.
  - Results come back through queued signals.
- **Every change to a note goes through a `Command`.** That is what makes undo/redo work and what tells autosave what is dirty. No direct mutations.
- **Atomic writes** (`QSaveFile`). **Never modify** imported original PDFs or images.
- **File format changes** require all three:
  - a bumped `format_version`;
  - a migration;
  - a round-trip test.
- **Errors:**
  - no exceptions across module boundaries;
  - fallible functions return `std::expected<T, Error>`;
  - `Q_ASSERT` only for real invariants.
- **Dependencies and licenses:**
  - the app is GPL-3.0-or-later: every dependency must be GPL-3.0-compatible, preferably permissive or LGPL (see ARCHITECTURE §2);
  - ask before adding any new dependency.
- **Strings:**
  - every user-visible string goes through `qsTr()` / `tr()` and lives in `app`;
  - `core` returns error codes, never text;
  - English source strings, Italian translation in `i18n/`.
- **Language:** code, comments, docs and commit messages are in English. Commits follow Conventional Commits.
- **The repository stays neutral:** no personal names (the GitHub account in URLs and the App ID is the only exception) and no references to other note-taking apps.
- **Every source file starts with** `// SPDX-License-Identifier: GPL-3.0-or-later` (`#` in CMake files).

## How to work

- Proceed by milestone and, within a milestone, in small steps that compile.
- At the end of each step: build, tests and clang-format all clean, with no compiler warnings.
- When touching input, rendering or gestures, ask for a test on a real 2-in-1 and say exactly what to try: pressure, the stylus eraser tip, a resting palm, pinch.
- Stroke performance matters more than anything else. If a choice makes latency worse, flag it.
- For visuals, follow the approved screen designs and `docs/DESIGN.md`. Don't invent new screens without asking.
