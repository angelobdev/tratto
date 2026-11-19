# Tratto

Handwritten notes and PDF annotation, built for laptops with a stylus.

Tratto is a native desktop app for writing by hand, annotating PDFs and images, and keeping notes organized in groups. It runs fully offline, with no accounts.

> **Status:** early development. Nothing is usable yet. See the milestones in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md#11-milestones).

## Planned features

- **Pen** with pressure, **highlighter**, **eraser** (whole stroke or partial) and keyboard **text**.
- **Lasso and rectangle selection:** move, resize, recolor, copy.
- **PDF and image annotation:** the original file is never modified.
- **Export** to PDF, PNG and JPG.
- **Paper:** blank, ruled, grid or dotted pages, in several colors and formats.
- **Theme:** light and dark, with a custom accent color.
- **Tablet mode** for stylus-and-touch use on 2-in-1 laptops.
- **Autosave.**

Linux comes first; Windows and macOS will follow.

## Building

Requirements: Qt ≥ 6.8 (Base, Declarative, SVG, PDF, Tools), CMake ≥ 3.21, Ninja and a C++23 compiler (GCC 13+ or Clang 17+).

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/src/app/tratto
```

Distribution packages:

- **Arch:** `cmake ninja qt6-base qt6-declarative qt6-svg qt6-webengine qt6-tools`
- **Fedora:** `cmake ninja-build qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtsvg-devel qt6-qtpdf-devel qt6-qttools-devel`
- **Ubuntu/Debian:** `cmake ninja-build qt6-base-dev qt6-declarative-dev qt6-svg-dev qt6-pdf-dev qt6-tools-dev qt6-l10n-tools`

## Translations

English is the source language. Translations live in [`i18n/`](i18n). After changing UI strings, refresh them with:

```bash
cmake --build build --target update_translations
```

## Documentation

- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md): stack, structure, data model, milestones.
- [`docs/DESIGN.md`](docs/DESIGN.md): colors, sizes, typography.

## License

[GPL-3.0-or-later](LICENSE).
