# Known limitations

Kept honest and current. An empty section is better than an optimistic one.

## Current state

The project is in Phase A (research). No format code exists. Nothing can be
claimed as working.

## Structural limitations of Version 1

Out of scope by design (specification §22): full synthesis/additive editor,
automatic categorization, AI classification, cloud or online services, KRA
graphical editing, ME-1 E/F support, automatic duplicate cleanup, direct MIDI
communication, live audition, hardware discovery.

No deduplication subsystem (§21). A trivial exact-match warning may exist if it
falls out of internal comparison, but it is not a feature and not a release
criterion.

Physical deletion of source files is deliberately not implemented (§24).
"Remove from workspace" never touches the file system.

## Environment

Qt 6 is not currently installed on the development machine, so the GUI target is
skipped by default. See [`TOOLCHAIN.md`](TOOLCHAIN.md).

## Formats

No parser or serializer exists, so every format sits at implementation level
`Unsupported` on all three axes (Parsing, Writing, Conversion) — including
FAT12, whose format is publicly documented but not yet implemented.

Research evidence is tracked separately and is currently `Observed` for KA1 and
KAA, `Documented` for FAT12, and `None` for everything else. See
[`FORMAT_NOTES.md`](FORMAT_NOTES.md). Support is never claimed in the UI for
anything below `GoldenTested`.
