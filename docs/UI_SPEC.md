# UI specification (Phase B)

Layout question resolved by [`OPEN_QUESTIONS.md` Q5](OPEN_QUESTIONS.md)
(2026-08-23): the specification wins over the prototype where they disagreed.

The interaction model here is binding for implementation. Sections marked
*to be written* are still open, but nothing already stated may be changed
silently.

## Approved visual direction

From `prototype/gui/GUI_HANDOFF.md`:

- modern dark graphite UI, restrained gold accent
- no retro-LCD imitation, no 1990s dense-librarian aesthetic
- conventional Windows desktop application behaviour, not a bespoke shell

## Main navigation (specification §2)

Five top-level destinations:

`Singles` · `Multis` · `Disk Images` · `SysEx` · `Settings`

`Multis` is a **top-level destination**, not a third tab inside the Singles bank
workspace. The prototype grouped Bank A / Bank D / Multi into one switch; that
grouping does not survive Q5.

## Singles screen

### Layout (Q5)

Three columns:

| Region | Content |
| --- | --- |
| left | Library / imported presets |
| center | Bank A **and** Bank D |
| right | Inspector |

**Responsive rule.** The center column has two modes, chosen automatically and
overridable by the user:

| Condition | Center column |
| --- | --- |
| Viewport width ≥ 1920 px **and** density is Compact or Normal | **Dual-bank**: Bank A and Bank D side by side, both fully visible |
| Viewport width < 1920 px, **or** density is Detailed | **Single-bank**: one workspace with an `A` / `D` switch |

Dual-bank is the default at the target resolution of 1920 × 1080. The switch
from the prototype survives only as this fallback.

When the layout collapses to single-bank, the inactive bank remains a live drop
target via the `A` / `D` switch header, so cross-bank drag-and-drop does not
require a mode change first.

### Card model (specification §7)

Each slot card shows at minimum:

- slot identifier in `A001` / `D001` form
- preset name
- compact source information (originating file / container)
- compact size or capacity contribution where useful

Density levels, cycled with `Ctrl + Wheel`:

| Density | Intent |
| --- | --- |
| **Compact** | Maximum slots visible; identifier and name only |
| **Normal** | Default; identifier, name, source |
| **Detailed** | Adds size/capacity and validation state — forces single-bank layout |

Capacity state is displayed on the bank header at all densities, per
[Q3](OPEN_QUESTIONS.md): used/free budget always visible, and a warning banner
when an unverified capacity model is exceeded.

### Format confidence in the UI (Q4)

Wording is derived from the format module's `VerificationLevel`, never
hard-coded. Anything below `GoldenTested` is labelled experimental at the point
of action — in the export dialog and in the log, not only in an About box. Write
actions for formats below `GoldenTested` are disabled unless explicitly enabled
in Settings.

## Interaction (specification §13, §14)

### Keyboard

`Ctrl+C` `Ctrl+X` `Ctrl+V` `Ctrl+Z` `Ctrl+Y` `Ctrl+Shift+Z` `Ctrl+A` `Ctrl+O`
`Ctrl+S` `Ctrl+Shift+S` `Ctrl+F` `F2` `Delete` `Esc`

`Cut` is safe: pressing `Ctrl+X` never destroys the source. Removal happens only
as part of a successful paste/move transaction.

### Selection

Ctrl-click for discontinuous selection, Shift-click for ranges, lasso selection
by dragging on empty canvas.

### Mouse

| Input | Action |
| --- | --- |
| Wheel | Scroll |
| Shift + Wheel | Horizontal scroll where applicable |
| Ctrl + Wheel | Change card density |
| Mouse 4 / Mouse 5 | Navigation history Back / Forward |
| Middle button | Unassigned in V1 (reserved for audition in V1.5) |

Ordinary wheel movement never modifies stored data. No destructive action is
bound to an auxiliary mouse button.

### Command dispatch

Keyboard, context menu, toolbar and drag-and-drop dispatch the **same** command
objects. There is no second implementation for any entry point — see
[`ARCHITECTURE.md`](ARCHITECTURE.md). This is what makes Undo/Redo behave
identically regardless of how an action was triggered.

### Drag-and-drop

Windows Explorer drops are accepted by the Library, both bank workspaces, the
Multi workspace and the Disk Image workspace (specification §12). Dropping a KAA
offers explicit choices — open as bank, insert contained presets, add to Library
— rather than picking one silently. Occupied destinations are never silently
overwritten.

## Reference prototype

`prototype/gui/index.html` — open directly in a browser, no build step. It is
**interaction and design reference only**; its mock data handling must not be
ported into production code, and its single-workspace bank switch is superseded
by Q5.

## To be written

- Multi workspace layout and the dependency view
  (`Multi → Part/Zone → Bank/Slot → Single`)
- Disk Image workspace
- SysEx workspace
- Settings
- Progress and cancellation presentation for long operations
- Error presentation (specification §35: source filename, detected format,
  offset, expected vs. actual — never raw exception text as the only diagnostic)
