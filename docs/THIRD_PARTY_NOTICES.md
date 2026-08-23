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
| test framework | pending | pending | See OPEN_QUESTIONS Q6 |

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

Not yet chosen. See [`OPEN_QUESTIONS.md` Q2](OPEN_QUESTIONS.md).
