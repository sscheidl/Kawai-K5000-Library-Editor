# UI specification (Phase B)

**Status: draft.** Blocked on [`OPEN_QUESTIONS.md` Q5](OPEN_QUESTIONS.md) — the
specification and the approved prototype disagree on whether Bank A and Bank D are
shown simultaneously. This document is completed once that is decided.

## Approved visual direction

From `prototype/gui/GUI_HANDOFF.md`:

- modern dark graphite UI, restrained gold accent
- no retro-LCD imitation, no 1990s dense-librarian aesthetic
- conventional Windows desktop application behaviour

## Main navigation (specification §2)

`Singles` · `Multis` · `Disk Images` · `SysEx` · `Settings`

## Singles screen (specification §2)

| Region | Content |
| --- | --- |
| left | Library / imported presets |
| center | Bank A and Bank D |
| right | Inspector |

Target resolution 1920 × 1080 with both banks legible at once — pending Q5.

## Card model (specification §7)

Each slot card shows at minimum: `A001` / `D001` slot identifier, preset name,
compact source information, compact size/capacity information where useful.

Density levels: **Compact · Normal · Detailed**, switched with `Ctrl + Wheel`.

## Interaction (specification §13, §14)

Standard Windows shortcuts (`Ctrl+C/X/V/Z/Y/A/O/S/F`, `Ctrl+Shift+Z`,
`Ctrl+Shift+S`, `F2`, `Delete`, `Esc`), Ctrl-click for discontinuous selection,
Shift-click for ranges, lasso selection on empty canvas.

Wheel scrolls, `Shift+Wheel` scrolls horizontally, `Ctrl+Wheel` changes density.
Mouse 4/5 are navigation Back/Forward. Middle mouse stays unassigned in V1
(reserved for audition in V1.5). Ordinary wheel movement never modifies stored
data.

Keyboard, context menu, toolbar and drag-and-drop dispatch the same commands —
see [`ARCHITECTURE.md`](ARCHITECTURE.md).

## Reference prototype

`prototype/gui/index.html` — open directly in a browser, no build step. It is
**interaction and design reference only**; its mock data handling must not be
ported into production code.

## To be written

- Multi workspace layout and the dependency view (`Multi → Part/Zone → Bank/Slot → Single`)
- Disk image workspace
- SysEx workspace
- Settings
- Capacity display (depends on Q3)
- Format confidence labelling in the UI (depends on Q4)
- Progress and cancellation for long operations
