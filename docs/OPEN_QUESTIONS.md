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

**Status: open. Blocks completion of `UI_SPEC.md`.**

Specification §2 requires: *"left: Library / imported presets, center: Bank A and
Bank D, right: Inspector"* and *"Bank A and Bank D should be visible
simultaneously at a normal 1920×1080 desktop resolution."*

The approved prototype v3 (`prototype/gui/GUI_HANDOFF.md`) instead describes *"a
single central workspace with switch: Bank A / Bank D / Multi"* — one bank visible
at a time.

These cannot both hold.

**Recommendation.** The specification wins, with the prototype's switch retained
as a fallback: A and D side by side by default at >= 1920 px, falling back to the
switched single-workspace view at narrower widths or at Detailed card density.
Multi stays its own destination in the main navigation, since §2 lists it as a
separate section rather than a third tab of the bank workspace.

**Decision:** _pending_

---

## Q6 — Test framework

**Status: open.** Golden-file and binary round-trip testing is the dominant use
case.

**Recommendation.** Catch2 v3 via CMake `FetchContent`, pinned to a release tag.
Good binary comparison and data-driven cases, no runtime dependency in the
shipped application. GoogleTest is the equally defensible alternative. Either way
it is recorded in `THIRD_PARTY_NOTICES.md`.

**Decision:** _pending_

---

## Q7 — Repository visibility and license

**Status: open.** Q2 is satisfied either way, but the license choice depends on
whether this becomes a public repository. No `LICENSE` file exists yet, so no
redistribution terms are currently granted.

**Decision:** _pending_
