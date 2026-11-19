# Tratto — Architecture

> Version 0.2 · 2026-11-19
> This document is the source of truth for technical decisions. When a decision changes, update it here in the same commit.
> Visual tokens live in [`DESIGN.md`](DESIGN.md).

## 1. Goals and constraints

- Native app for handwritten notes and for annotating PDFs and images.
- **Linux first**, especially 2-in-1 laptops with a stylus. Windows and macOS follow; no choice may rule them out.
- Fully local and offline: no accounts, no cloud.
- One app, two ways of using it: **desktop** (mouse and keyboard) and **tablet** (stylus and fingers, large controls).
- Features: pen, highlighter, eraser (whole stroke / partial), keyboard text, selection (lasso / rectangle), images; standalone notes or notes in groups; customizable paper; export to PDF, PNG and JPG; light/dark theme with a user-chosen accent color; autosave.
- UI language: English, with an Italian translation from day one.

## 2. Stack

| Area | Choice | Notes |
| --- | --- | --- |
| Language | C++23 | `std::expected` for errors. GCC 13+, Clang 17+, MSVC 19.38+. |
| UI framework | Qt 6, **minimum 6.8 (LTS)** | Qt Quick + QML for the interface, C++ for the canvas and all logic. |
| Controls | Qt Quick Controls, **Basic** style customized | Basic is the most themeable style; the look comes from `DESIGN.md`, not from the platform. |
| Build | CMake ≥ 3.21 + Ninja | `qt_add_qml_module` for QML modules. |
| Rendering | `QPainter` | One renderer for screen tiles **and** export: what you see is what you export. |
| Live stroke | Qt Quick scene graph (`QSGGeometryNode`) | GPU-drawn, updated every frame. |
| PDF display | Qt PDF (`QPdfDocument`, PDFium-based) | Ships with Qt (LGPLv3). |
| PDF export | `QPdfWriter` + **qpdf** (Apache-2.0) | qpdf overlays annotations onto the original pages (see §7). Added in M4. |
| Images | `QImage` / `QImageReader` / `QImageWriter` | PNG, JPG, WEBP via Qt image plugins. |
| Library index | Qt SQL with the SQLite driver | Index and cache only; can be rebuilt. |
| Serialization | CBOR (`QCborValue`) for pages, JSON (`QJsonDocument`) for the manifest | Both in QtCore. |
| Spatial index | Uniform grid per page, in `core` | For eraser and selection hit-testing. Revisit if profiling says so. |
| Paths | `QStandardPaths::AppDataLocation` | XDG on Linux, native folders on Windows and macOS. |
| IDs | UUIDv7 (time-ordered) | Small generator in `core` (`QUuid::createUuidV7` needs Qt 6.9). |
| Logging | `QLoggingCategory` | One category per module (`tratto.core`, `tratto.storage`, …). |
| Tests | Qt Test + CTest | |
| Translations | Qt Linguist (`qsTr` / `tr`, `.ts` files) | English source strings, `it` translation. |

**Licensing.** Tratto is **GPL-3.0-or-later**. App ID: `io.github.angelobdev.Tratto`.

- Qt is used under LGPLv3, with dynamic linking. LGPL modules (QtCore, QtGui, QtQml, QtQuick, QtQuick Controls, QtSvg, QtPdf, QtSql, QtTest) are preferred.
- Every dependency must be GPL-3.0-compatible (LGPL, MIT, BSD, Apache-2.0, GPL-3.0).
- Prefer permissive or LGPL dependencies, so components stay reusable.

## 3. Repository layout

```text
tratto/
├── CMakeLists.txt
├── src/
│   ├── core/       # tratto_core: data model, stroke geometry, commands, undo/redo. QtCore only.
│   ├── render/     # (from M1) tratto_render: QPainter rendering of pages and objects (tiles, thumbnails, export)
│   ├── storage/    # (from M2) tratto_storage: note bundles, SQLite index, autosave, migrations
│   ├── pdf/        # (from M4) tratto_pdf: PDF import, rasterization, annotated export (worker thread)
│   └── app/        # executable: QML UI, canvas item, input handling, controllers exposed to QML
│       └── qml/
├── tests/          # one Qt Test executable per library
├── i18n/           # tratto_it.ts (+ tratto_en.ts for English plural forms)
├── data/           # icons, .desktop file, metainfo
├── docs/
└── scripts/
```

Dependency rule, enforced in CMake:

- `core` links only `Qt6::Core`.
- `render` and `storage` add `Qt6::Gui` (and `Qt6::Sql` for `storage`), but **never** `Qt6::Quick` or `Qt6::Qml`.
- Their tests run with `QT_QPA_PLATFORM=offscreen`, with no display.
- All the hard logic lives in these libraries and is tested there: geometry, eraser, selection, file format.

## 4. Data model (`core`)

**Units: typographic points (pt, 1/72 inch)**, origin at the top-left of the page. A4 = 595.28 × 841.89 pt. PDF export is therefore 1:1 and zoom is only a view transform.

```cpp
struct Note {
    NoteId id;
    QString title;
    std::optional<GroupId> group;        // nullopt = standalone note
    QDateTime created, modified;
    PaperStyle defaultPaper;             // used for new pages
    std::vector<PageId> pages;           // page order
    std::optional<SourceDoc> source;     // imported PDF or image
};

struct Group { GroupId id; QString name; Rgba color; quint32 order; };

struct PaperStyle {
    Background background;
    Rgba paperColor;
    PageFormat format;
    Orientation orientation;             // ignored for Continuous
};
// Background: Blank | Ruled{spacingMm} | Grid{spacingMm} | Dots{spacingMm}   (std::variant)
enum class PageFormat { A4, Letter, Continuous }; // Continuous: A4 width, height grows with content

struct Page {
    PageId id;
    QSizeF size;                         // pt
    PageKind kind;                       // Paper{PaperStyle} | PdfPage{index} | Image{AssetId}
    std::vector<Object> objects;         // order = drawing order within its layer
};

using Object = std::variant<Stroke, TextBox, ImageObject>;

struct StrokePoint { float x, y, pressure; };   // pressure 0..1

struct Stroke {
    ObjectId id;
    StrokeTool tool;                     // Pen | Highlighter
    Rgba color;
    float width;                         // base width, pt
    std::vector<StrokePoint> points;     // already smoothed and simplified
    QRectF bbox;                         // cache, recomputed on load
};

struct TextBox     { ObjectId id; QRectF rect; QString text; TextStyle style; };
struct ImageObject { ObjectId id; AssetId asset; QRectF rect; Rotation90 rotation; };
```

**Undo/redo:** every change is a `Command` (`AddObjects`, `RemoveObjects`, `ReplaceObjects`, `Transform{ids, affine}`, `EditText`, `AddPage`, …) with `apply` and `revert`. There is one in-memory stack per note, with a size limit. **The document is never modified outside a command.** Commands are also how autosave learns what is dirty.

We use our own command stack rather than `QUndoStack`, because `QUndoStack` lives in QtGui and its model doesn't fit `core`.

## 5. Input and tools

**Event sources (canvas item):**

- **Stylus:**
  - Qt Quick items have no `tabletEvent()` hook, so the canvas installs an event filter on its `QQuickWindow`.
  - It receives `QTabletEvent` (position, pressure, tilt, pointer type) before Qt Quick synthesizes mouse events.
  - Events outside the canvas are passed through.
  - The **eraser tip** (`PointerType::Eraser`) activates the eraser while it is in use.
- **Mouse:** constant pressure 0.5. Mouse events synthesized from touch are ignored.
- **Fingers:**
  - With **"Stylus only"** on (the default in tablet mode), a finger never draws.
  - One finger scrolls, two fingers zoom, a two-finger tap undoes and a three-finger tap redoes.

**Stroke pipeline:**
1. Raw samples → drop points that are too close (< 0.5 px on screen).
2. Live preview: the stroke in progress is a scene-graph node updated every frame, without going through the tile cache.
3. On release:
   - smoothing;
   - simplification (Ramer–Douglas–Peucker, low tolerance);
   - a variable-width outline computed from pressure, *perfect-freehand* style, filled as a path.
4. The stroke becomes an `AddObjects` command.

**Tools:**

- **Highlighter:**
  - constant width, color with alpha ≈ 0.45;
  - drawn on a layer **below** the ink;
  - "straight line" option: the stroke becomes a segment.
- **Eraser, whole stroke:** hit-test the stroke segments against the eraser circle using the spatial grid → `RemoveObjects`.
- **Eraser, partial:**
  - strokes are split at the edge of the circle → `ReplaceObjects`;
  - "highlighter only" option.
- **Lasso selection:** a polygon; a stroke is selected when most of its points fall inside (configurable threshold).
- **Rectangle selection:** intersection with the bbox.
- **Selection actions:**
  - move, resize with handles, recolor, copy/cut/duplicate/delete;
  - clipboard: internal MIME format + PNG for other apps.
- **Text:**
  - a tap creates a `TextBox`;
  - while editing, a QML `TextEdit` is overlaid on the canvas; when it loses focus the edit becomes a command;
  - rendered with `QPainter` / `QTextLayout`.
- **Images:**
  - from file, clipboard or drag and drop; copied into `assets/`;
  - move, resize, rotate by 90°; cropping comes later.

## 6. Rendering

**Layer order per page:** paper (color + pattern) → source (PDF raster or image) → inserted images → highlighter → ink → text → UI overlay (selection, handles, eraser cursor).

**On screen:**

- **Canvas item:** a custom `QQuickItem` that builds its own scene-graph nodes.
- **Virtualized pages:** only visible pages are drawn.
- **Tiles:**
  - each page is split into tiles (e.g. 512 px), painted with `QPainter` into `QImage`s **on worker threads** (`QThreadPool`);
  - the tiles are uploaded as textures in `updatePaintNode()`.
- **Zoom:**
  - quantized levels (0.5 / 0.75 / 1 / 1.5 / 2 / 3 / 4);
  - during a pinch the existing textures are scaled; tiles are re-rasterized when the gesture ends.
- **Live stroke:** a `QSGGeometryNode` (triangle strip of the outline) that never waits for tiles.
- **Invalidation:** changing objects invalidates only the tiles that intersect their bbox.

**Performance targets:**

- The live stroke lags the stylus by no more than one frame.
- Scrolling stays at 60 fps on a page with ~5,000 strokes.

## 7. PDF and images

**Import:**

- The original PDF is **copied** into the bundle (`assets/source.pdf`) and never modified.
- One `Page` per PDF page, with that page's size.
- PDF rasterization runs on **one dedicated worker thread** that owns the `QPdfDocument` instances. It handles requests ("page i at scale s") and returns `QImage`s.
- An imported image either becomes a page sized to the image or is inserted into the current note.

**Export:**

- **Paper note → PDF:** `QPdfWriter` + `QPainter`, fully vector (strokes as paths, text as text).
- **Annotated PDF → PDF:** the original content must be preserved (selectable text), with annotations added as vectors. The plan, to be validated with a spike in M4:
  - **A (preferred):** render each page's annotations with `QPdfWriter` into a temporary PDF, then use **qpdf** to overlay it onto the original page as a form XObject.
  - **B (last resort):** rasterize.
- **PNG/JPG:** one page or all pages (one file per page), at 150 or 300 DPI.
- **Signature:** signing with the pen for now; saved signatures may come later.

## 8. Storage and autosave

Data folder: `QStandardPaths::AppDataLocation` (on Linux, `~/.local/share/tratto/`).

```text
library.sqlite              # index: notes, groups, titles, dates, ordering, search text
notes/<note-id>/
├── manifest.json           # metadata, format_version, page order, source
├── pages/<page-id>.cbor    # page objects
├── assets/                 # inserted images, source PDF/image
└── thumb.png               # thumbnail of the first page, for the library
```

- **Bundles are the source of truth.**
  - SQLite is only a cache: if it is missing or corrupt, it is rebuilt by scanning `notes/`.
- **Autosave:**
  - every command marks the page it touches as dirty;
  - a save happens 500 ms after the last change, at most every 5 s during continuous writing;
  - it also always happens when leaving the editor, closing the window and on suspend.
- **Writes:**
  - they run on a **dedicated I/O thread**;
  - they are **atomic**: `QSaveFile` (tmp + fsync + rename);
  - only dirty pages are rewritten;
  - the thumbnail is regenerated after saving, rate-limited.
- **Versioning:** `format_version` in the manifest, migrations in `storage`, and a round-trip test for every version.
- **Trash:** soft delete (`deleted_at`), emptied automatically after 30 days.
- **Crash recovery:** leftover temporary files from a crash are ignored and removed at startup.
- **UI indicator:** "Saving…" while changes are pending, "Saved" once the write completes.

## 9. Navigation and interface

- **Window:** an `ApplicationWindow` with a `StackView`: **Library** (root) → **Editor**. Going back immediately flushes pending changes.
- **Library:**
  - a sidebar (248 px): All notes, Recent, Standalone notes, Groups, Trash, Settings;
  - content: a `GridView` of thumbnails (or a list);
  - search queries SQLite.
- **New note:** a dialog with a live preview of the paper.
- **Settings:** a dialog with sections.
- **Theme:**
  - a QML singleton `Theme` holds the `DESIGN.md` tokens for light and dark;
  - light / dark / system follows `QStyleHints::colorScheme`;
  - the accent color is a property of `Theme`, so changing it updates the UI live;
  - in dark mode, accent text and icons use the lightened accent (see `DESIGN.md`).
- **Tablet mode:**
  - an editor state that changes density and layout;
  - tools move from a horizontal row of 44 px buttons to a floating vertical bar of 56 px buttons, with larger popovers.
- **Strings:**
  - every user-visible string goes through `qsTr()` / `tr()`, in `app` only;
  - `core` returns error codes, never text.

## 10. Platforms and distribution

- **Linux:** Flatpak on the KDE runtime (which ships Qt), plus qpdf bundled.
- **Windows:**
  - `windeployqt` + installer;
  - Qt reads the stylus through Windows Ink (`WM_POINTER`).
- **macOS:**
  - `macdeployqt` → `.app` / `.dmg`;
  - stylus via `NSEvent` tablet events.
- **CI:** GitHub Actions builds and tests on all three platforms from M6.

## 11. Milestones

| # | Content | Done when |
| --- | --- | --- |
| **M0** | CMake project, window, Library ↔ Editor navigation with fake data, light/dark theme + accent color, i18n wiring (en + it) | The app opens, navigation works, the theme and language switch live |
| **M1** | Canvas item: one page with paper, pressure pen (stylus) and mouse, zoom/pan, undo/redo | Writing with the stylus on a 2-in-1 feels fluid |
| **M2** | Persistence: bundles, SQLite index, autosave, library with thumbnails and groups, New note dialog | Close and reopen without losing anything; killing the app causes no corruption |
| **M3** | Highlighter, eraser (both modes), lasso/rectangle selection with actions, text, images | Every tool in the design works |
| **M4** | PDF/image import, annotated-PDF export spike, PDF/PNG/JPG export | An exported annotated PDF opens correctly in other readers |
| **M5** | Tablet mode, Stylus only and gestures, full settings, polish | Comfortable to use with stylus and fingers only |
| **M6** | Flatpak packaging, then Windows/macOS builds and CI | Installable from a file |

## 12. Open questions

- **Automatic tablet mode:** Qt doesn't expose the "laptop folded" state.
  - For now: a manual toggle plus a heuristic (last input from touch/stylus and no key pressed for N seconds).
  - To investigate: the `SW_TABLET_MODE` switch via libinput/evdev (needs permissions) and the equivalent APIs on Windows.
- **Handwriting search (OCR):** out of scope for now.
- **Saved, reusable signatures.**
- **Sync between devices:** out of scope for now.
