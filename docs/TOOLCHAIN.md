# Toolchain

Verified on the development machine on **2026-08-23**. Re-verify after any
Visual Studio, CMake or Qt update and update this table.

## Verified present

| Component | Version / path | Status |
| --- | --- | --- |
| CMake | 4.4.2 — `C:\Program Files\CMake\bin\cmake.exe` | OK (project requires ≥ 3.24) |
| Visual Studio Build Tools 2022 | 17.14.37314.3 — `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools` | OK — **primary toolchain** |
| MSVC toolset (2022) | 14.44.35207, `cl.exe` 19.44.35227.0, Host x64 / target x64 | OK — C++20 available |
| Visual Studio Build Tools 18 | 18.9.12105.275, MSVC toolset 14.51.36231 | Present, **not used by default** |
| Windows SDK | 10.0.26100.0 | OK |
| MSBuild | `…\2022\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe` | OK |
| Ninja | bundled with both Build Tools installations under `Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja` | Available, not on `PATH` |
| Git | 2.53.0.windows.1 | OK |
| **Qt** | **6.10.3**, `C:\Qt\6.10.3\msvc2022_64` | OK — Core, Gui and Widgets present, `windeployqt.exe` present |

### Configure smoke test

```
cmake --preset windows-msvc-debug
```

Result: **Configuring done / Generating done**, `CMAKE_CXX_COMPILER_ID = MSVC`
`19.44.35227.0`, `Qt6 found: 1`. The `src/gui` and `src/app` subdirectories are
still reported as skipped because no GUI code exists yet — Qt detection is what
this test establishes.

## Missing — action required

Nothing. The toolchain is complete for the work in progress.

## Qt

Installed: **6.10.3**, MSVC 2022 x64, at `C:\Qt\6.10.3\msvc2022_64`.

The presets carry that path in `CMAKE_PREFIX_PATH`, so an ordinary
`cmake --preset windows-msvc-debug` finds Qt with no extra flags. Override it on
the command line if Qt lives somewhere else on another machine:

```
cmake --preset windows-msvc-debug -DCMAKE_PREFIX_PATH=<path>/msvc2022_64
```

`CMakeLists.txt` requires Qt ≥ 6.5 and only the Widgets component. If Qt is
absent the GUI target is skipped with a warning and core, CLI and tests still
build — that path stays supported so CI needs no Qt.

**Version choice.** 6.10.3 is not an LTS release. Qt's LTS patch releases (the
6.8.x series) are commercial-only in the Online Installer, so the open-source
installer offers the current feature releases instead. That is fine here: the
project uses Qt Widgets and nothing version-sensitive. Pin deliberately if a
future Qt update ever breaks the GUI.

`windeployqt.exe` from the same installation is used for packaging (§39, "Qt
deployment dependencies are correctly packaged").

## Notes and pitfalls

- **Do not build inside `%TEMP%`.** The MSBuild FileTracker fails there with
  `MSB8029` / `FTK1011`. Build directories live under `build/` in the repository
  (git-ignored).
- Two MSVC toolsets are installed side by side. The presets pin
  `Visual Studio 17 2022` so both agents produce comparable build output. Switching
  to the 18 toolset is a deliberate, documented change, not a default.
- `cl.exe` is not on the global `PATH` — this is normal. CMake locates it through
  the generator; a developer command prompt is only needed for manual compiler calls.

## Dependency policy

Per specification §1: reuse what is installed, add only what is actually required,
and document every additional dependency in
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md). No new language ecosystem
(.NET, Java, Electron, Node) may be introduced.
