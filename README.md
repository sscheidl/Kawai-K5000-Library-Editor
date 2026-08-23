# K5000 Librarian & Gotek Builder

Offline librarian, bank builder, format converter and Gotek/FlashFloppy disk-image
manager for the **Kawai K5000S / K5000R**.

> This is **not** a synthesizer parameter editor. Additive/harmonic editing is
> explicitly out of scope — see [`docs/MASTER_SPECIFICATION.md`](docs/MASTER_SPECIFICATION.md) §22.

**Status: reading KA1 and KAA.** The KA1 parser and KAA extraction are
implemented, read-only, and `GoldenTested` against the private corpus. Writing is
`Unsupported` for every format and stays that way until each rule behind it is
verified. Subsystems are added one at a time in the order fixed by
[decision Q1](docs/OPEN_QUESTIONS.md):

```
KA1 → KAA → Bank A/D workspace → Single SysEx → FAT12 → IMG → Deep Extract
→ KC1 → KCA → Multi workspace/references → integration → hardening
```

## What it will do (Version 1)

| Area | Scope |
| --- | --- |
| Single presets | KA1, KAA, K5000 Single SysEx — import, export, conversion |
| Multi presets | KC1, KCA, Multi SysEx where verified |
| Bank workspaces | Bank A and Bank D, 128 slots each, full editing + Undo/Redo |
| Mass import/export | Recursive directories, Explorer drag-and-drop, per-slot filenames |
| Disk images | 1.44 MB FAT12 `.IMG` create / open / edit / validate, Extract All, Deep Extract |
| Gotek | Build a ready-to-use FlashFloppy image from the current workspace |

Reserved for Version 1.5: direct MIDI transfer to the hardware and preset audition.

## Technology

- C++20, CMake ≥ 3.24
- Qt 6 Widgets (desktop GUI; not QML)
- MSVC / Windows 11 x64 as the primary target
- Core libraries build and test **without** Qt

See [`docs/TOOLCHAIN.md`](docs/TOOLCHAIN.md) for the verified local environment
and what still needs installing.

## Build

```bash
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

Without a Qt 6 installation the GUI target is skipped automatically; core, CLI and
tests still build:

```bash
cmake --preset core-only
```

## Repository layout

```
src/core/            canonical data model, command/undo system, diagnostics
src/formats/         ka1/ kaa/ kc1/ kca/ sysex/ — parsers and serializers
src/disk/fat12/      FAT12 / 1.44 MB IMG implementation
src/library/         library index, scanning, import/export orchestration
src/gui/             Qt 6 Widgets UI (no binary parsing code lives here)
src/cli/             k5000cli — diagnostic/test executable on the same core
src/app/             desktop application entry point
tests/               unit/ integration/ golden/
testdata/            reference material (see docs/TEST_MATRIX.md)
prototype/gui/       approved non-functional HTML/JS interaction reference
docs/                specification and working documentation
```

## Documentation

| Document | Purpose |
| --- | --- |
| [`docs/MASTER_SPECIFICATION.md`](docs/MASTER_SPECIFICATION.md) | The authoritative V1 specification |
| [`docs/OPEN_QUESTIONS.md`](docs/OPEN_QUESTIONS.md) | Decisions required before implementation |
| [`docs/TOOLCHAIN.md`](docs/TOOLCHAIN.md) | Verified build environment |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Layering and module boundaries |
| [`docs/FORMAT_NOTES.md`](docs/FORMAT_NOTES.md) | Binary format research, verified vs. unverified |
| [`docs/UI_SPEC.md`](docs/UI_SPEC.md) | GUI specification (Phase B) |
| [`docs/TEST_MATRIX.md`](docs/TEST_MATRIX.md) | Coverage per format and operation |
| [`docs/KNOWN_LIMITATIONS.md`](docs/KNOWN_LIMITATIONS.md) | Honest limitations |
| [`docs/HARDWARE_ACCEPTANCE.md`](docs/HARDWARE_ACCEPTANCE.md) | Real K5000S / Gotek test log |
| [`docs/THIRD_PARTY_NOTICES.md`](docs/THIRD_PARTY_NOTICES.md) | Dependencies and attribution |

## Core rule

Parsing and reserialization must never silently alter musical data. Imported files
are read-only source material. Unknown and reserved bytes are preserved, not
normalized. Malformed input is reported, never auto-repaired.

## License

**None, provisionally.** The repository is private and ships no `LICENSE` file,
so default copyright applies and no redistribution terms are granted. This is an
explicitly provisional decision — see
[`docs/OPEN_QUESTIONS.md` Q7](docs/OPEN_QUESTIONS.md).
