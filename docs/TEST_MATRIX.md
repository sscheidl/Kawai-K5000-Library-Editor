# Test matrix

Rows are taken from the final acceptance matrix (specification §43). Every row
must eventually carry an honest value in all five columns — "works" without
evidence is not an acceptable entry (§43).

Legend: `—` not started · `partial` · `yes` · `n/a`

| Capability | Implemented | Automated test | Golden/reference test | Hardware test | Remaining risk |
| --- | --- | --- | --- | --- | --- |
| KA1 import | — | — | — | — | Format at *Observed* only |
| KA1 export | — | — | — | — | Byte preservation on rename unproven |
| KAA import | — | — | — | — | Pointer base address unknown |
| KAA export | — | — | — | — | Capacity model unknown |
| KAA → KA1 mass export | — | — | — | — | Byte-exactness of extraction unproven |
| KAA → SYX mass export | — | — | — | — | SysEx framing unknown |
| KA1/SYX mass import | — | — | — | — | — |
| Bank A editing | — | — | — | n/a | — |
| Bank D editing | — | — | — | n/a | — |
| Bank capacity validation | — | — | — | — | See OPEN_QUESTIONS Q3 |
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
