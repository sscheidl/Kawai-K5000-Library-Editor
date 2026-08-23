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

### Notes

- No production format code exists yet. Nothing is claimed as supported.
