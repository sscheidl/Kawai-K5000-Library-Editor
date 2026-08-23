# Open questions and decisions

Decisions are binding once recorded here with a date. Reopening one is itself a
recorded decision, not a silent change.

---

## Q1 — V1 scope cut: does Multi move to V1.1?

**DECIDED 2026-08-23 — Multi stays a V1 goal, but is sequenced after the
Single/IMG core. No guessing if the format is still unclear.**

Consequence for the implementation order (amends specification §40):

```
project/toolchain → GUI prototype → KA1 → KAA → Bank A/D workspace
→ Single SysEx → FAT12 → IMG GUI → Deep Extract
→ KC1 → KCA → Multi workspace/references
→ integration → release hardening
```

Multi therefore comes after the Gotek workflow is working, not before it. The
"no guessing" clause is operative: if KC1/KCA research does not reach
`GoldenTested`, Multi write support does not ship — it stays at whatever
`VerificationLevel` the evidence supports, and the release notes say so. That is
a Version 1 limitation, not a reason to invent field meanings.

---

## Q2 — Reference files for `testdata/`

**DECIDED 2026-08-23 — Real commercial files are used as a private local golden
corpus and are never committed. Public tests run on synthetic and freely
redistributable fixtures.**

Implementation:

1. `testdata/` in the repository contains only self-generated fixtures and files
   whose redistribution is unambiguously permitted.
2. The external corpus lives outside the repository and is **read-only**. Its
   location is configured through the CMake cache variable
   `K5000_REFERENCE_CORPUS` or the equivalent environment variable — never
   hard-coded, never written to.
3. `testdata/REFERENCE_MANIFEST.csv` records relative path, size and SHA-256 for
   each corpus file, so a golden test can assert it is reading the file it
   expects without the file being present in the repository.
4. Golden tests that need the corpus **skip with an explanatory message** when it
   is absent. A clean public checkout passes.
5. `.gitignore` and CI guard against accidental commits of
   `.KA1/.KAA/.KC1/.KCA/.KRA/.SYX/.IMG` outside the permitted fixture set.

Corpus location in use:
`D:\Backup\Backup_System\Music Production\Audio Hardware\Kawai K5000S\Presets`
— **read-only, never modified or deleted by this project or its tests.**

---

## Q3 — Capacity model: warning or hard gate?

**DECIDED 2026-08-23 — Only verified limits are hard gates. Unconfirmed capacity
models produce warnings.**

| Capacity model level | Behaviour on "export as valid K5000 bank" |
| --- | --- |
| `GoldenTested` / `HardwareVerified` | Hard gate. Export refused; explicit, logged override required. |
| `Observed` / `Experimental` | Prominent warning with a written reason. Export allowed. |
| `Unsupported` | No capacity claim made; export allowed, no capacity statement in the log. |

Used/free budget is always displayed in the UI, and the capacity state is always
recorded in the export log regardless of level.

---

## Q4 — Verification flag per format

**DECIDED 2026-08-23 — `VerificationLevel` is anchored technically in the core,
tracked separately for Parsing, Writing and Conversion.**

```
Unsupported → Experimental → Observed → GoldenTested → HardwareVerified
```

| Level | Meaning |
| --- | --- |
| `Unsupported` | Not implemented, or understood too poorly to act on. No semantic claim. |
| `Experimental` | Implemented from a documented source; not yet reproduced against reference files. Off by default for writing. |
| `Observed` | Reproduced consistently across reference files, no documentary confirmation. |
| `GoldenTested` | Round-trip and golden-file tests pass against the reference corpus. |
| `HardwareVerified` | Confirmed on the real K5000S / Gotek and logged in `HARDWARE_ACCEPTANCE.md`. |

Rules:

- Each format module exposes a descriptor carrying the three axis levels plus an
  evidence reference. The axes move independently — parsing routinely leads
  writing, and conversion is gated by the weaker of its two endpoints.
- The GUI and the export log derive their wording from the descriptor. Nothing
  below `GoldenTested` is presented as plain "supported".
- **Write paths below `GoldenTested` are disabled unless explicitly enabled** by
  the user, and every such export is logged with its level.
- `k5000cli formats` prints the full table, so the reviewing agent can diff
  claimed support against actual test coverage in one command.
- A test asserts that every `GoldenTested` claim has a corresponding round-trip
  and golden test, and every `HardwareVerified` claim has a dated row in
  `HARDWARE_ACCEPTANCE.md`. The level cannot be raised without evidence.

---

## Q5 — Prototype contradicts the specification on Bank A/D layout

**DECIDED 2026-08-23 — The specification wins. Bank A and Bank D are shown
simultaneously from 1920 px; below that, and at Detailed card density, the view
falls back to a switchable single-bank workspace. Multi remains its own top-level
navigation destination.**

Background: specification §2 requires *"left: Library / imported presets, center:
Bank A and Bank D, right: Inspector"* with both banks visible at 1920×1080. The
approved prototype v3 (`prototype/gui/GUI_HANDOFF.md`) instead described a single
central workspace switching between Bank A / Bank D / Multi. The prototype's
switch is retained as the narrow-viewport and high-density fallback rather than
as the primary model.

Multi is **not** a third tab of the bank workspace — §2 lists `Multis` as its own
section of the main navigation.

Consequence: [`UI_SPEC.md`](UI_SPEC.md) is unblocked.

---

## Q6 — Test framework

**DECIDED 2026-08-23 — Catch2 v3 via CMake `FetchContent`, pinned to a fixed
release tag.**

Pinned to **v3.7.1**. Changing the pin is a deliberate, committed change, never
a floating branch — golden tests must not shift underneath a release because a
dependency moved.

`FetchContent` is declared with `FIND_PACKAGE_ARGS`, so a Catch2 already
installed on the machine is used instead of a download. `K5000_FETCH_CATCH2=OFF`
disables the download path entirely for offline builds; tests are then skipped
with an explanatory message rather than failing configuration.

Recorded in [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md). Catch2 is a test
dependency only and is not linked into the shipped application.

---

## Q7 — Repository visibility and license

**DECIDED 2026-08-23 — The repository is hosted privately on GitHub as
`Kawai-K5000-Library-Editor` and is licensed **MIT**.**

This supersedes the earlier provisional decision to ship no `LICENSE` at all.
The intent recorded with the change is that the project will be freely
available to everyone, and MIT is the licence that delivers that with the least
friction: any use, including commercial, in exchange for keeping the copyright
notice.

Consequences:

- Private hosting is an access decision, not a licensing one. The MIT grant is
  already in place, so making the repository public later needs no licence
  change and no history rewrite.
- Q2 stays in force regardless: the preset corpus is third-party, partly
  commercial, and is never committed. The MIT licence covers this project's
  source code only, which `LICENSE` states explicitly.
- Qt remains under the LGPL and is unaffected. See `THIRD_PARTY_NOTICES.md`.
- The copyright holder is recorded as `sscheidl`, taken from the git
  configuration. Replace it with a full legal name if the project is ever
  published or distributed in a context where that matters.
