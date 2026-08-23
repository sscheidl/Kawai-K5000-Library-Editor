# Test data

Policy fixed by [`docs/OPEN_QUESTIONS.md` Q2](../docs/OPEN_QUESTIONS.md),
decided 2026-08-23.

## What lives here

`testdata/fixtures/` — **only** self-generated fixtures and files whose
redistribution is unambiguously permitted. These are committed and are what CI
runs on. Keep them small and purposeful: a truncated file, a deliberately
corrupt pointer table, a minimal valid bank.

Everything else is blocked by `.gitignore`. K5000 binary extensions
(`.KA1 .KAA .KC1 .KCA .KRA .KB1 .SYX .IMG`) are ignored repository-wide and
un-ignored only under `testdata/fixtures/`, so a stray corpus file cannot be
committed by accident.

## The private reference corpus

Real preset material — including commercial content — stays **outside** the
repository and is treated as **read-only source material**. It is never
committed, never modified, never deleted by this project or its tests
(specification §24, §26).

Point the build at it to enable golden tests:

```bash
cmake --preset core-only -DK5000_REFERENCE_CORPUS="D:/path/to/corpus"
```

or set the `K5000_REFERENCE_CORPUS` environment variable. When it is unset or
missing, golden tests **skip with an explanatory message** — a clean public
checkout still passes.

## Manifest

`REFERENCE_MANIFEST.csv` records relative path, size and SHA-256 for each corpus
file a test depends on. A golden test asserts the hash before reading, so it
fails loudly on the wrong file rather than silently comparing against something
else. The manifest is committed; the files it describes are not.

Not yet generated — it is written alongside the first golden test.
