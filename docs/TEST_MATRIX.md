# Test matrix

Rows are taken from the final acceptance matrix (specification §43). Every row
must eventually carry an honest value in all five columns — "works" without
evidence is not an acceptable entry (§43).

Legend: `—` not started · `partial` · `yes` · `n/a`

| Capability | Implemented | Automated test | Golden/reference test | Hardware test | Remaining risk |
| --- | --- | --- | --- | --- | --- |
| KA1 import | — | — | — | — | Evidence `Corroborated`; ADD/PCM flag inside the source descriptor still unknown |
| KA1 export | — | — | — | — | Checksum algorithm unresolved — blocks every write path except byte-exact copy |
| KAA import | — | — | — | — | Base = min non-zero pointer, validated on 4072 patches; gap contents in fragmented banks unknown |
| KAA export | — | — | — | — | Whether the instrument requires address-ordered patches is unconfirmed |
| KAA → KA1 mass export | — | — | — | — | Byte-exactness corroborated on 2195 pairs; needs a golden test to pin it |
| KAA → SYX mass export | — | — | — | — | SysEx framing unknown |
| KA1/SYX mass import | — | — | — | — | — |
| Bank A editing | — | — | — | n/a | — |
| Bank D editing | — | — | — | n/a | — |
| Bank capacity validation | — | — | — | — | 128 slots + 131072-byte budget known; warning-only until `GoldenTested` (Q3) |
| KC1 | — | — | — | — | See OPEN_QUESTIONS Q1 |
| KCA | — | — | — | — | See OPEN_QUESTIONS Q1 |
| KCA mass export | — | — | — | — | See OPEN_QUESTIONS Q1 |
| Multi reference management | — | — | — | — | See OPEN_QUESTIONS Q1 |
| Single SysEx | — | — | — | — | Unverified |
| Bank SysEx | — | — | — | — | May be dropped from V1 |
| Multi SysEx | — | — | — | — | May be dropped from V1 |
| Windows Explorer drag-in | — | — | n/a | n/a | — |
| Internal drag-and-drop | — | — | n/a | n/a | — |
| Drag-out to Explorer | — | — | n/a | n/a | SHOULD, not blocking |
| Keyboard shortcuts | — | — | n/a | n/a | — |
| Mouse-wheel behaviour | — | — | n/a | n/a | — |
| Mouse Back/Forward | — | — | n/a | n/a | — |
| Undo/Redo | — | — | n/a | n/a | — |
| FAT12 reading | — | — | — | n/a | — |
| FAT12 writing | — | — | — | — | Independent tool cross-check required |
| IMG opening | — | — | — | n/a | — |
| IMG creation | — | — | — | — | — |
| IMG modification | — | — | — | — | Round-trip corruption risk |
| IMG Extract All | — | — | — | n/a | — |
| IMG Deep Extract | — | — | — | n/a | — |
| Workspace persistence | — | — | n/a | n/a | — |
| Source-file safety | — | — | — | n/a | Must be asserted by test, not by policy |
| Hardware KA1 validation | n/a | n/a | n/a | — | — |
| Hardware KAA validation | n/a | n/a | n/a | — | — |
| Hardware KCA validation | n/a | n/a | n/a | — | — |
| Gotek IMG validation | n/a | n/a | n/a | — | — |

## Reference corpus

Golden tests run against an external, read-only corpus (see
[`OPEN_QUESTIONS.md` Q2](OPEN_QUESTIONS.md)). Tests that require it are **skipped
with an explanatory message** when it is unavailable, so a clean public checkout
still passes. Reference files supplied by the user are never modified by tests
(specification §26).

Corpus inventory as surveyed on 2026-08-23: 963 `.KA1`, 76 `.KAA` (all exactly
134 660 bytes), 42 `.SYX`, 13 `.KCA`, 5 `.KRA`, 15 `.IMG` (all exactly
1 474 560 bytes).

The corpus is **private and never committed** — it contains commercial preset
content. Its location comes from the `K5000_REFERENCE_CORPUS` CMake cache
variable or the matching environment variable. Public tests run on the
synthetic and freely redistributable fixtures in `testdata/fixtures/`; see
[`../testdata/README.md`](../testdata/README.md).

## Verification levels

Each row's status is backed by the per-format `VerificationLevel`
(`Unsupported → Experimental → Observed → GoldenTested → HardwareVerified`,
tracked separately for Parsing, Writing and Conversion — decision Q4). A row
reaches `GoldenTested` only with a passing round-trip and golden test, and
`HardwareVerified` only with a dated row in
[`HARDWARE_ACCEPTANCE.md`](HARDWARE_ACCEPTANCE.md). A test asserts this
correspondence, so the levels cannot drift ahead of the evidence.
