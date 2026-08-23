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

### Notes

- No production format code exists yet. Nothing is claimed as supported.
