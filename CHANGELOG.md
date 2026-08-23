# Changelog

All notable changes to this project are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); the
project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Repository scaffold: CMake project, presets, shared compile options, CI,
  issue and pull-request templates, contribution guide.
- Documentation set required by specification §37, plus `docs/TOOLCHAIN.md`
  and `docs/OPEN_QUESTIONS.md`.
- Approved GUI prototype v3 imported to `prototype/gui/`.
- First Phase A evidence pass on KA1 and KAA recorded in `docs/FORMAT_NOTES.md`.
- `tools/check-no-corpus-binaries.sh` and a CI job that reject tracked K5000
  binary files outside `testdata/fixtures/`, and oversized fixtures. Ignore
  rules do not cover already-tracked files, so this checks the index directly.
- CI guard that fails the build if the obsolete four-level confidence scale
  reappears in the repository.

### Decided

- **Q1** Multi (KC1/KCA) stays a Version 1 goal but is sequenced after the
  Single/IMG core; no guessing where the format is unclear.
- **Q2** Real commercial preset files are used as a private local golden corpus
  and are never committed. Public tests run on synthetic fixtures under
  `testdata/fixtures/`. Corpus path comes from `K5000_REFERENCE_CORPUS`.
- **Q3** Only verified capacity limits act as hard export gates; unconfirmed
  models produce warnings.
- **Q4** `VerificationLevel` is anchored in the core, tracked separately for
  Parsing, Writing and Conversion:
  `Unsupported → Experimental → Observed → GoldenTested → HardwareVerified`.
- **Q5** The specification wins over the prototype on bank layout: Bank A and
  Bank D are shown simultaneously from 1920 px, with a switchable single-bank
  fallback below that and at Detailed density. Multi stays a top-level
  navigation destination. `docs/UI_SPEC.md` is unblocked and written.
- **Q6** Catch2 v3, pinned to v3.7.1, via `FetchContent` with
  `FIND_PACKAGE_ARGS`; `K5000_FETCH_CATCH2=OFF` for offline builds.
- **Q7** *(provisional)* The repository stays private and ships no `LICENSE`.
  Default copyright applies; to be revisited before any public release.

### Changed

- Research evidence and implementation status are now tracked on two separate
  scales. Research evidence (`None / Observed / Documented / Corroborated`)
  records what is understood; `VerificationLevel` records what the code does.
  Since no parser exists, every format is `Unsupported` on all three axes.
- The obsolete four-level confidence scale is removed repository-wide and a CI
  guard fails the build if it reappears.

### Researched

Phase A, KA1 and KAA, from the digitalsynth.net analysis plus verification
against the private corpus (963 KA1, 76 KAA). Reproducible with
`tools/research/kaa_probe.py`.

- Patch layout: `size = 82 + 86*S + 806*A` (common, source descriptors, ADD
  wave kits). Holds for 938 of 963 KA1 files; the other 25 are recorded as
  malformed-input fixtures.
- Source count lives at common offset `0x33`; name at `0x28`, 8 bytes; `0x27`
  is constant `0x00`.
- The whole payload is 7-bit, matching the claim that KAA patches are in the
  SysEx data format.
- **A KA1 file is the patch payload verbatim** — 2195 byte-identical matches
  between KAA slots and same-name KA1 files. `KAA -> KA1` can therefore be a
  byte-exact extraction, satisfying section 5 without re-serialization.
- KAA container: 3584-byte pointer table (128 patches x 7 big-endian pointers),
  end-of-data pointer at `0x0E00`, 131072-byte data region from `0x0E04`;
  total 134660, matching all 76 banks. Base address = minimum non-zero pointer,
  validated by decoding names for all 4072 extracted patches.
- Capacity model: 128 slots **and** a 131072-byte budget, the latter binding in
  practice.
- Checksum for byte `0x00` is **unresolved**; eight candidate algorithms were
  tested and all failed. Recorded as a negative result. This blocks every write
  path except byte-exact copy.

### Implemented

- **Read-only KA1 parser** (`src/formats/ka1/`). Strict bounds checking, no
  silent repair, unknown bytes preserved, structured diagnostics carrying
  offset plus expected and actual conditions. Validation states: `Valid`,
  `ChecksumMismatch`, `UnsupportedVariant`, `StructurallyAmbiguous`,
  `Truncated`, `Malformed`.
- **KAA bank reader and byte-exact extraction** (`src/formats/kaa/`). Pointers
  locate patch starts; length comes exclusively from the patch structure.
- **Core diagnostics and verification model** (`src/core/`).
- **`k5000cli`** with `formats`, `info` and `extract`. Extraction writes via a
  temporary file and never overwrites.
- **38 tests**: synthetic-fixture unit tests that need no corpus, plus
  corpus-gated golden tests that skip with an explanation when none is set.
- **`testdata/REFERENCE_MANIFEST.csv`** — anonymous fixture ids, sizes,
  SHA-256 and expected parse status for all 1039 corpus files. No payloads, no
  original filenames. Cross-checked against the parser by a golden test.
- **`docs/CODEX_HANDOFF.md`** — reproducible commands and the specific claims
  to attack.

### Researched

Phase B, verified against the private corpus and reproducible from
`tools/research/`:

- **Checksum rule confirmed**:
  `(sum(common[1..81]) + sum(active source descriptors) + 0xA5) & 0x7F`,
  **excluding** the ADD wave kits, which carry their own. 937 of 937
  structurally complete files match; 0 mismatches. This supersedes the earlier
  negative result, which had not tested the correct scope.
- **ADD/PCM detection confirmed**: wave kit number at source-relative
  `+28/+29`, two 7-bit bytes high first, `512` means additive. Agrees with the
  independent length-derived count in 937 of 938 files; the one contradiction
  is reported as `StructurallyAmbiguous` rather than resolved by guessing.
- **KAA pointer mapping confirmed**:
  `file_offset = 0x0E04 + (pointer - smallest non-zero pointer)`, validated by
  checking the patch checksum at all 4072 computed offsets across 76 of 76
  banks. The base is bank-dependent, not constant.
- **Stale ADD-kit pointers found**: 28 slots carry a wave kit pointer for a
  source that does not exist, left by a larger patch that previously occupied
  the slot. Sizing from the pointer table is therefore wrong as well as unsafe
  — structural sizing reproduces 2210 standalone KA1 files byte for byte
  against 2195 for pointer-derived sizing.

### Notes

- Writing remains `Unsupported` for every format, asserted by a unit test.
  Rename is blocked because the reserved bytes are unknown, not because the
  checksum is.
- Nothing is `HardwareVerified`. No generated or extracted file has been loaded
  on a real K5000S.
