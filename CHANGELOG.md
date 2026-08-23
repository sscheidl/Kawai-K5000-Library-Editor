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

### Notes

- No production format code exists yet. Nothing is claimed as supported.
- Q5 (Bank A/D layout: specification vs. prototype), Q6 (test framework) and
  Q7 (repository visibility and license) remain open.
