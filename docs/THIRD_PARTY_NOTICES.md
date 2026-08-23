# Third-party notices

Per specification §1, every dependency beyond the pre-existing toolchain is
recorded here with its purpose and license.

## Runtime dependencies

| Component | Version | License | Purpose |
| --- | --- | --- | --- |
| Qt 6 (Widgets) | >= 6.5 | LGPLv3 / commercial | Desktop GUI |

Qt is used under the LGPL. The application links Qt dynamically and ships the Qt
DLLs produced by `windeployqt`; Qt sources are not modified.

## Build and test dependencies

| Component | Version | License | Purpose |
| --- | --- | --- | --- |
| CMake | >= 3.24 | BSD-3-Clause | Build system |
| MSVC Build Tools 2022 | 17.14 | Microsoft | Compiler toolchain |
| Catch2 | v3.7.1 (pinned) | BSL-1.0 | Test framework (OPEN_QUESTIONS Q6). Fetched via CMake `FetchContent`, or used from a local install. Test-only — not linked into the shipped application. |

## Documentation sources

Format research draws on publicly available documentation and prior community
work, cited in [`FORMAT_NOTES.md`](FORMAT_NOTES.md): Kawai's own documentation and
MIDI implementation, the digitalsynth.net analysis of K5000 patch data files,
Edisyn, KSynthLib / k5ktool, and the historical `KA1toKAA` / `KAAtoKA1` /
`kaanalyz` tools. No third-party source code is copied into this repository.

## Reference material

The Kawai K5000 preset corpus used for golden testing is third-party content —
partly commercial — held locally and **not redistributed** by this repository. See
[`OPEN_QUESTIONS.md` Q2](OPEN_QUESTIONS.md).

## This project's own license

**None, provisionally.** Per [`OPEN_QUESTIONS.md` Q7](OPEN_QUESTIONS.md)
(2026-08-23, explicitly provisional) the repository stays private and ships no
`LICENSE` file, so default copyright applies and no redistribution rights are
granted. This is expected to be revisited before any public release.

Qt's LGPL obligations above are unaffected by that decision.
