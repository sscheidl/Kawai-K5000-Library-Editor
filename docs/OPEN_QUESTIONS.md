# Open questions

Decisions required from the project owner. Each entry states the question, why it
matters, and a recommendation. Nothing below is decided until this file records a
decision and a date.

---

## Q1 — V1 scope cut: does Multi move to V1.1?

**Context.** Specification §11 makes KC1/KCA and Multi reference management
mandatory for V1. The reference corpus contains only 13 KCA files and no loose
KC1, and the Multi formats are currently at confidence level *Unknown*. Multi
reference management (§11: detect affected Multis when a Single moves, offer to
update references, make that undoable) is one of the more intricate parts of the
whole application, and it depends on the Single bank model already being final.

**Why it matters.** Multi sits between "Bank A/D workspace" and "FAT12" in the
implementation order (§40). If it stays in V1 and turns out to need extended
research, it blocks the Gotek workflow — which is the feature with the clearest
practical payoff.

**Recommendation.** Split it: keep **KC1/KCA parse, display and byte-exact
pass-through into disk images** in V1, and move **Multi editing plus automatic
reference updating** to V1.1. That keeps Multis usable in the Gotek workflow
without gating V1 on the hardest correctness problem in the project.

**Decision:** _pending_

---

## Q2 — Reference files for `testdata/`

**Context.** A substantial local corpus exists at
`D:\Backup\…\Kawai K5000S\Presets` (963 KA1, 76 KAA, 42 SYX, 13 KCA, 5 KRA,
15 IMG). It is treated as **read-only source material** and is never modified by
this project or its tests (§26). Part of it is commercial third-party content
(e.g. an LFO Store bundle) and part is from public repositories.

**Why it matters.** Committing that material into a public GitHub repository would
redistribute commercial presets. But golden-file tests are worthless without real
reference data.

**Recommendation.** Three tiers:

1. `testdata/` in the repository contains only **self-generated fixtures** plus
   any file whose redistribution is unambiguously permitted — small, and safe to
   publish.
2. A **manifest** (`testdata/REFERENCE_MANIFEST.csv`: relative path, size,
   SHA-256) describes the external corpus. Tests that need it are skipped with a
   clear message when the corpus is absent, so a clean public checkout still
   passes.
3. The corpus location is configured once via a CMake cache variable
   (`K5000_REFERENCE_CORPUS`) or an environment variable, never hard-coded.

Open sub-question: should the repository be **public or private**? If private,
tier 1 can be more generous. This also gates the license choice (no `LICENSE`
file exists yet).

**Decision:** _pending_

---

## Q3 — Capacity model: warning or hard gate?

**Context.** Specification §8 says "Prevent export as *valid K5000 bank* if
verified capacity limits are exceeded". The capacity rules themselves are not yet
researched, and the fixed 134 660-byte KAA size suggests the real constraint is a
data-region budget rather than the 128-slot count.

**Why it matters.** A hard gate built on a capacity model that is still at
confidence level *Observed* will block legitimate exports. A pure warning risks
producing banks the hardware rejects.

**Recommendation.** Tie the behaviour to the confidence level, and show it:

- Capacity model **Verified** → hard gate on "export as valid K5000 bank", with an
  explicit, logged override.
- Capacity model **Observed / Documented** → prominent warning plus a written
  reason, export allowed.
- Always display used/free budget in the UI, and always record the state in the
  export log.

This satisfies §8 once research completes, without blocking work before it does.

**Decision:** _pending_

---

## Q4 — Verified/Unverified flag per format

**Context.** Specification §6 and §30 forbid claiming support for unverified
variants. [`FORMAT_NOTES.md`](FORMAT_NOTES.md) already defines four confidence
levels (Verified / Documented / Observed / Unknown).

**Why it matters.** The flag needs to be a real, machine-readable property of each
format module — otherwise it decays into a documentation claim that the code
contradicts, which is exactly what §41 tells the reviewing agent not to trust.

**Recommendation.** Make it structural rather than documentary:

- Each format module exposes a `FormatSupport` descriptor carrying its confidence
  level, the evidence reference, and separate read/write capability flags. Read
  support may legitimately be ahead of write support.
- The GUI derives its wording from that descriptor. Anything below *Verified* is
  labelled experimental in the UI and in export logs, and non-`Verified` **write**
  paths are off unless explicitly enabled.
- `k5000cli formats` prints the table, so the reviewing agent can diff claimed
  support against actual test coverage in one command.
- A test asserts that every `Verified` claim has a corresponding round-trip and
  golden test — the flag cannot be raised without evidence.

**Decision:** _pending_

---

## Q5 — Prototype contradicts the specification on Bank A/D layout

**Not on the original list, but it blocks `UI_SPEC.md`.**

Specification §2 requires: *"left: Library / imported presets, center: Bank A and
Bank D, right: Inspector"* and *"Bank A and Bank D should be visible
simultaneously at a normal 1920×1080 desktop resolution."*

The approved prototype v3 (`prototype/gui/GUI_HANDOFF.md`) instead describes *"a
single central workspace with switch: Bank A / Bank D / Multi"* — one bank visible
at a time.

These cannot both hold. Which one wins?

**Recommendation.** The specification, with the prototype's switch retained as a
density mode: show A and D side by side by default at ≥ 1920 px, and fall back to
the switched single-workspace view at narrower widths or when the user picks a
detailed card density. Multi stays a separate destination in the main navigation
(§2 lists it as its own section, not as a third tab of the bank workspace).

**Decision:** _pending_

---

## Q6 — Test framework

No framework has been chosen. Golden-file and binary round-trip testing is the
dominant use case.

**Recommendation.** Catch2 v3 via CMake `FetchContent`, pinned to a release tag.
It handles binary comparison and data-driven cases well and adds no runtime
dependency to the shipped application. GoogleTest is the equally defensible
alternative. Either way it gets recorded in `THIRD_PARTY_NOTICES.md`.

**Decision:** _pending_
