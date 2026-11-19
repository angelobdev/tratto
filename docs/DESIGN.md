# Tratto — Design tokens

Values taken from the approved screen designs. The layout follows those screens: when in doubt, the screens win over this file. If something changes, update this file.

In the app these tokens live in the QML `Theme` singleton; this file is their reference.

## Interface colors

| Role | Light | Dark |
| --- | --- | --- |
| App background / canvas area | `#F4F4F5` | `#18181B` (canvas `#111113`) |
| Sidebar | `#EDEDEF` | `#1C1C1F` |
| Surfaces (bars, cards, dialogs) | `#FFFFFF` | `#202024` |
| Secondary surface (chips, segmented controls) | `#F0F0F2` | `#2A2A2F` |
| Border | `#E4E4E7` | `#2E2E33` |
| Strong border (secondary buttons, inputs) | `#D4D4D8` | `#3F3F46` |
| Text | `#18181B` | `#F4F4F5` |
| Secondary text / icons | `#3F3F46` | `#D4D4D8` |
| Muted text | `#5B5B63` | `#A1A1AA` |
| Danger (Delete) | `#C92A2A` | `#FFA8A8` |

Every text/background pair has a contrast ratio of at least 4.5:1. Keep it that way.

## Accent color (user-chosen)

- **Default:** `#3B5BDB`.
- **Presets:** Blue `#3B5BDB`, Violet `#7048E8`, Raspberry `#C2255C`, Orange `#C2410C`, Green `#237032`, Teal `#0F766E`, Sky `#1971C2`, Graphite `#495057`, plus a custom color.
- **accent** (fills: primary button, switches):
  - the color as is, with `#FFFFFF` text on top;
  - the presets guarantee ≥ 4.5:1 against white;
  - for a custom color with contrast < 4.5:1 against white, use `#18181B` text.
- **accent soft** (background of the active tool and of the selected item): accent at alpha 0.12 in light mode, 0.26 in dark mode.
- **accent on dark** (accent text and icons in dark mode): accent mixed 45% with white (`c + (255 − c) × 0.45` per channel).
- **Selection ring:** `0 0 0 2px <surface>, 0 0 0 4px <accent>` (in dark mode use *accent on dark*).

## Group colors

- **Palette:** Violet `#AE3EC9`, Orange `#E8590C`, Green `#0CA678`, plus an extendable palette.
- **Display:** an 8–10 px square with a 2–3 px radius.

## Ink

- **Pen:**
  - colors: Black `#18181B`, Blue `#1E3A8A`, Red `#E03131`, Green `#2B8A3E`, Orange `#E8590C`, Violet `#7048E8`, plus a "More colors" picker;
  - on dark paper (Slate) the default black becomes `#F1F3F5`.
- **Highlighter** (alpha 0.45): Yellow `#FFD43B`, Green `#8CE99A`, Pink `#FFA8C5`, Sky `#74C0FC`, Orange `#FFC078`.
- **Pen widths** (base width in pt; pressure scales it between 0.4× and 1.4×): fine 1.0 · medium 2.0 · thick 3.5.
- **Highlighter widths:** 8 · 12 · 18 pt.
- **Eraser radius (partial):** 5 · 8 · 12 pt.

## Paper

- **Backgrounds:** Blank · Ruled 6 mm · Ruled 8 mm · Grid 5 mm · Grid 10 mm · Dots 5 mm.
- **Paper colors:** White `#FFFFFF` · Ivory `#FBF6EA` · Pearl gray `#F1F3F5` · Sage `#EEF3EA` · Slate `#2B2D31` · custom.
- **Lines on light paper:**
  - ruled `rgba(30,58,138,0.17)`;
  - grid `rgba(24,24,27,0.10)`;
  - dots `rgba(24,24,27,0.28)`, radius 1 px at 100% zoom.
- **Lines on dark paper** (luminance < 110/255): ruled `rgba(255,255,255,0.16)`, grid `0.12`, dots `0.30`.
- **Top margin:** ruling starts below a blank top margin (~96 px at a 680 px page width).
- **Formats:** A4 · Letter · Continuous. Portrait/landscape orientation (not for Continuous).

## Sizes and shapes

| Element | Desktop | Tablet |
| --- | --- | --- |
| Top bar | 56 px | 72 px |
| Tool row | 60 px, horizontal, centered below the top bar | floating vertical bar on the left |
| Tool button | 44 × 44, radius 10 | 56 × 56, radius 16 |
| Color swatches | 22 px dot in a 34 × 44 area | 36 px dot in a 52 px area |
| Library sidebar | 248 px | — |
| Pages panel | 184 px (side to be decided) | hidden, opened from a button |

- **Radii:** buttons and inputs 10 · cards 12 · dialogs 16 · floating pills 12 (desktop) / 20–22 (tablet).
- **Spacing:** 4 px grid. Typical gaps: 4, 8, 12, 16, 24.
- **Touch targets:** at least 44 px (52 px or more in tablet mode).
- **Shadows:**
  - cards: `0 1px 3px rgba(24,24,27,.06)` + 1 px border;
  - pills and popovers: `0 8px 24px rgba(24,24,27,.10)`;
  - page: `0 10px 30px rgba(24,24,27,.08)`.

## Typography

- **UI:**
  - font: Inter (bundled; fallback: system font);
  - body 13–15 px;
  - library title 26/700;
  - dialog title 19/700;
  - section labels 12/600 uppercase, letter spacing 0.06em.
- **Note text** (Text tool): Inter, Serif or Monospace; 16 pt by default.

## Icons

- **Style:** outline icons with a 1.8 stroke and rounded corners, 20–22 px on desktop and 24–26 px in tablet mode.
- **Source:** [Lucide](https://lucide.dev) SVGs (ISC license), bundled in `data/icons/`, recolored at runtime to the theme's icon color.
