# K5000 Librarian & Gotek Builder – GUI Prototype v3

This package is a **GUI-only prototype** for the planned Kawai K5000S/R librarian and Gotek/FlashFloppy image builder.

There is intentionally **no real KA1/KAA/KC1/KCA/SysEx/FAT12 implementation yet**.

## Run

Open `index.html` in a current desktop browser.

No build step, package manager, web server or external dependency is required.

## Prototype coverage

### Bank workspace

- Switch between Bank A, Bank D and Multi
- 16 / 32 / 64 / 128 slot views for A/D
- 64-slot maximum for Multi
- Ctrl-click multi-selection
- Shift-click range selection
- Lasso selection by dragging on empty canvas
- Drag a mock Library preset into A/D
- Ctrl+C / Ctrl+X / Ctrl+V
- Ctrl+Z / Ctrl+Y / Ctrl+Shift+Z
- Ctrl+A
- Ctrl+F
- F2
- Delete
- Context menu with:
  - Export Patch(es) as KA1
  - Convert Patch(es) to SysEx
  - Send Patch(es) to .IMG
  - Copy / Cut / Paste
  - Rename
  - Clear
- Whole-bank actions:
  - Convert Bank A/D to SysEx
  - Send Bank A/D to .IMG
- Multi equivalents use KC1/KCA wording where applicable

### Disk Images

- Mock 1.44 MB FAT12 image workspace
- Add/remove entries
- Explorer drop demo
- Extract All / Deep Extract placeholders
- Create IMG from Workspace placeholder

### SysEx

- Selection → SysEx workflow
- Whole bank → SysEx workflow

## V1 scope intent

The real application is planned as a C++20 / CMake / Qt 6 Widgets Windows application.

Primary V1 focus:

- KA1 / KAA
- KC1 / KCA
- SysEx file conversion
- Bank A / D / Multi management
- mass import/export
- Windows Explorer drag-and-drop
- FAT12 `.IMG` creation/editing/extraction
- Gotek workflow
- undo/redo and safe file handling

## V1.5 reserved

- direct MIDI connection to K5000
- send/receive banks and presets
- audition
- Space / middle-click audition
- optional auto audition

## Important for implementation agents

The HTML/JS code in this prototype is **interaction/design reference code only**.

Do not port its mock data handling into production code.

The production implementation should preserve the visual and interaction model while using properly separated C++ core/parser/serializer/GUI layers.
