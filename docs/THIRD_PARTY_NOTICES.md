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

## Vendored code

None. No third-party source code is copied into this repository.

- **Catch2** and **Qt** are consumed as external dependencies, not vendored.
  Catch2 is fetched at configure time; Qt is linked from a system installation.
- **`prototype/gui/`** is self-contained HTML, CSS and JavaScript, authored by
  the project owner with ChatGPT as the assisting tool, and confirmed as such.
  No third party holds rights in it. It loads no external library, contacts no
  remote host, and its only script tag points at its own `app.js`. It is
  interaction and design reference only and is not built into any shipped
  artefact.
- The K5000 format knowledge was derived from published documentation and from a
  reproduced `kaanalyz` output listing, then verified against files. The
  historical utilities' source code was **not** read and none of it is
  reproduced here. File formats are facts, not expression.

## Reference material

The Kawai K5000 preset corpus used for golden testing is third-party content —
partly commercial — held locally and **not redistributed** by this repository. See
[`OPEN_QUESTIONS.md` Q2](OPEN_QUESTIONS.md).

## This project's own license

**MIT**, per [`OPEN_QUESTIONS.md` Q7](OPEN_QUESTIONS.md) (2026-08-23). See
[`../LICENSE`](../LICENSE).

The grant covers this project's source code only. It does not extend to the Qt
libraries above, which stay under the LGPL, nor to the Kawai preset corpus,
which is third-party content held outside the repository.
