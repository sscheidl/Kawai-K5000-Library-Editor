# Codex handoff — KA1 parser and KAA extraction

**Review target:** commit `dd66e53`, branch `main`, repository
`sscheidl/Kawai-K5000-Library-Editor` (private).
**Handed over:** 2026-08-23.

**Subsystem ownership while this audit runs:** `src/formats/ka1/`,
`src/formats/kaa/`, `src/core/` and their tests are frozen. Claude Code will not
touch them until the audit reports, so that both agents are not reworking the
same subsystem at once (specification §0). Findings go into a report, not
directly into the code — Codex reports, Claude Code fixes, Codex re-verifies.

You are the independent reviewer for this phase (specification §41). Nothing
below is asserted to be correct. Verify it.

**Do not trust** the comments, the README, this document, the commit messages,
the test names, or the numbers quoted here. Inspect the implementation and
re-derive the numbers yourself. Where a claim and the code disagree, the code is
the fact and the claim is the bug.

## What this phase produced

| Area | Location |
| --- | --- |
| Research probes (Python) | `tools/research/*.py` |
| KA1 read-only parser | `src/formats/ka1/` |
| KAA reader and extraction | `src/formats/kaa/` |
| Core diagnostics and verification model | `src/core/` |
| Diagnostic CLI | `src/cli/main.cpp` |
| Unit tests, synthetic fixtures | `tests/unit/` |
| Corpus-gated golden tests | `tests/golden/` |
| Anonymized fixture manifest | `testdata/REFERENCE_MANIFEST.csv` |
| Format facts and evidence | `docs/FORMAT_NOTES.md` |

## Reproducible commands

The reference corpus is **private, read-only, and never committed**. Point the
tools at your own extracted copy. Nothing here writes to the corpus.

```bash
CORPUS=/path/to/extracted/corpus
```

Research probes — each prints its own statistics and exits non-zero on failure:

```bash
python tools/research/ka1_checksum_probe.py "$CORPUS"
python tools/research/ka1_source_probe.py "$CORPUS"
python tools/research/kaa_pointer_probe.py "$CORPUS"
python tools/research/kaa_probe.py "$CORPUS"
```

Build and test, without the corpus (public path — golden tests skip):

```bash
cmake --preset core-only -G "Visual Studio 17 2022" -A x64
cmake --build build/core-only --config Debug
ctest --test-dir build/core-only -C Debug --output-on-failure
```

With the corpus (golden tests run):

```bash
cmake --preset core-only -G "Visual Studio 17 2022" -A x64 -DK5000_REFERENCE_CORPUS="$CORPUS"
cmake --build build/core-only --config Debug
ctest --test-dir build/core-only -C Debug --output-on-failure
```

Regenerate the manifest and diff it — it must not change:

```bash
python tools/research/make_manifest.py "$CORPUS" --out testdata/REFERENCE_MANIFEST.csv
git diff --stat testdata/REFERENCE_MANIFEST.csv
```

Inspect individual files:

```bash
build/core-only/bin/Debug/k5000cli formats
build/core-only/bin/Debug/k5000cli info "$CORPUS/SOMEBANK.KAA"
build/core-only/bin/Debug/k5000cli info "$CORPUS/SOME.KA1"
```

## Claims to attack

Each of these is asserted somewhere in the code or docs. Try to break them.

### 1. The checksum rule and its scope

Claimed: `(sum(common[1..81]) + sum(active source descriptors) + 0xA5) & 0x7F`,
**excluding** the ADD wave kits.

- Is the summation range in `ka1::computeChecksum` actually what the docs say?
  Check the `subspan(1, end - 1)` arithmetic against an off-by-one.
- Does it hold on a corpus you assemble independently?
- The four files that fail their *kit* checksum while passing the *patch*
  checksum are used as evidence for the scope. Is that inference sound, or could
  those four be explained another way?
- `computeChecksum` returns 0 when the payload is too short. Can a caller reach
  that path and mistake the 0 for a real checksum?

### 2. ADD/PCM classification

Claimed: wave kit number at source-relative `+28/+29`, two 7-bit bytes high
first, `512` means additive.

- One corpus file contradicts the file-length cross-check. Confirm it is one and
  not more.
- The out-of-range value `8186` implies the high byte can exceed 7 bits'
  worth of plausible range. Does `(b[+28] << 7) | b[+29]` mis-decode anything?
- PCM wave numbers cluster in 341..511 and nothing below 341 was ever seen. That
  is unexplained. Does it hint the field is misread, or is it a property of the
  instrument's numbering?

### 3. Structural length, and Invariant A

Claimed: patch length comes only from `82 + 86*S + 806*A`, never from pointers.

- Grep for any arithmetic on two pointers that could reach a size.
- `tests/unit/test_kaa_bank.cpp` has two tests tagged `[invariant]`. Do they
  actually fail if you deliberately reintroduce pointer-delta sizing in
  `kaa::extractSlot`? Try it. A regression test that does not fail is decoration.
- Fragmented banks: are gaps ever included in a payload?

### 4. KAA pointer mapping

Claimed: `file_offset = 0x0E04 + (pointer - smallest non-zero pointer)`.

- The base includes ADD-kit pointers in the minimum, not just patch pointers.
  Is that right, or could a stale kit pointer drag the base below the true data
  start and shift every patch?
- Is `Bank::fileOffset` correct at both ends of the data region?
- Does anything read `bytes[offset]` before checking that `offset` is in range?

### 5. Bounds safety

- `ka1::parse` is handed a span. Confirm every read is guarded — particularly
  `waveOffset + 1` for the last source, and the kit subspans.
- `kaa::extractSlot` bounds parsing by the rest of the file. Can a crafted
  source count still read past the end?
- Feed the parser fuzzed and truncated input. It must never crash.

### 6. Source-file safety

- Confirm nothing in `tools/research/` or the test suite opens a corpus path for
  writing.
- `k5000cli extract` writes new files. Confirm it never overwrites, and that a
  failed write leaves no `.partial` behind.

### 7. Verification honesty

- `ka1::support()` and `kaa::support()` claim `GoldenTested` for parsing. Is
  that justified, given the golden tests skip without a corpus?
- Writing is `Unsupported` everywhere. Confirm no write path exists that
  bypasses that.
- `testdata/REFERENCE_MANIFEST.csv` is generated by the Python model and checked
  against the C++ parser. Are they actually independent, or does one derive from
  the other in a way that makes the cross-check circular?

## Known open items — do not report these as findings

They are already recorded in `docs/FORMAT_NOTES.md`:

- Most common-data bytes are unidentified and carried verbatim.
- The 806-byte ADD wave kit's internal layout is unknown.
- Which bytes are reserved for rename purposes is unknown; rename is blocked.
- PCM wave numbering is unexplained.
- Whether the instrument requires address-ordered patches in a bank is unknown.
- Single SysEx is a hypothesis only. KC1/KCA untouched.
- Nothing is hardware-verified.

## Classification

Use P0 / P1 / P2 / P3 as in `CONTRIBUTING.md`. For every P0 and P1 give the
source file and function, the trigger, an explanation, a reproducible test and a
recommended fix. A confirmed P0/P1 is not fixed until a regression test covers
it.

Data-corruption and byte-preservation defects are P0 by definition, including
any path where an extracted payload differs from the embedded bytes.
