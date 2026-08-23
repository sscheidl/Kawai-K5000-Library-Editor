# Format notes

Working document for Phase A. Specification §6/§30: never claim support for an
unverified variant, and never guess undocumented binary fields silently.

## Verification levels

The scale is defined in [`OPEN_QUESTIONS.md` Q4](OPEN_QUESTIONS.md) (decided
2026-08-23) and is anchored in the core as a `VerificationLevel`, tracked
**separately for Parsing, Writing and Conversion**:

```
Unsupported → Experimental → Observed → GoldenTested → HardwareVerified
```

| Level | Meaning |
| --- | --- |
| `Unsupported` | Not implemented, or understood too poorly to act on. No semantic claim; bytes are preserved verbatim. |
| `Experimental` | Implemented from a documented source; not yet reproduced against reference files. Writing off by default. |
| `Observed` | Reproduced consistently across reference files, no documentary confirmation yet. |
| `GoldenTested` | Round-trip and golden-file tests pass against the reference corpus. |
| `HardwareVerified` | Confirmed on the real K5000S / Gotek and logged in `HARDWARE_ACCEPTANCE.md`. |

The axes move independently — parsing routinely leads writing, and a conversion
is capped by the weaker of its two endpoints. Write paths below `GoldenTested`
are disabled unless the user explicitly enables them, and every such export is
logged with its level.

This document is the evidence record behind those levels. Raising a level here
without the corresponding test is exactly the kind of unbacked claim §41 tells
the reviewing agent not to trust.

## Per-format status

| Format | V1 scope | Parsing | Writing | Conversion | Notes |
| --- | --- | --- | --- | --- | --- |
| KA1 (single) | required | `Observed` | `Unsupported` | `Unsupported` | Header block + 8-char name located; body layout open |
| KAA (single bank) | required | `Observed` | `Unsupported` | `Unsupported` | 4-byte big-endian pointer table located; base address unknown |
| Single SysEx | required | `Unsupported` | `Unsupported` | `Unsupported` | Framing not yet examined |
| Bank SysEx | only if verifiable | `Unsupported` | `Unsupported` | `Unsupported` | May be dropped from V1 |
| KC1 (multi) | required, after Single/IMG core | `Unsupported` | `Unsupported` | `Unsupported` | Not yet examined (Q1) |
| KCA (multi bank) | required, after Single/IMG core | `Unsupported` | `Unsupported` | `Unsupported` | Not yet examined (Q1) |
| Multi SysEx | only if verifiable | `Unsupported` | `Unsupported` | `Unsupported` | May be dropped from V1 |
| KRA (arpeggio) | pass-through only | `Unsupported` | `Unsupported` | n/a | Stored in images byte-exact, never parsed in V1 |
| FAT12 / 1.44 MB IMG | required | `Experimental` | `Unsupported` | n/a | Publicly specified format; nothing implemented yet |

Bank capacity is tracked on the same scale, currently `Unsupported`. Per
[Q3](OPEN_QUESTIONS.md), it produces warnings rather than a hard export gate
until it reaches `GoldenTested`.

## Sources

Primary documentation available locally:

- *Making Sense of Kawai K5000 Patch Data Files* — digitalsynth.net (PDF, in the
  local reference collection). **Not yet read in detail** — this is the next
  research task and is expected to resolve most KA1/KAA fields.
- Kawai K5000 MIDI implementation documentation
- Wizoo K5000 books (2 PDFs, local)

Cross-checks planned against independent implementations: Edisyn's K5000 support,
KSynthLib / k5ktool, and the historical `KA1toKAA` / `KAAtoKA1` / `kaanalyz` tools.
Where two sources disagree, the disagreement is recorded rather than resolved by
preference.

## Reference corpus

Private local golden corpus (OPEN_QUESTIONS Q2): **read-only, never committed,
never modified by tests**. Public tests use synthetic fixtures. See [`TEST_MATRIX.md`](TEST_MATRIX.md) for how it is used.

| Type | Count | Sizes |
| --- | --- | --- |
| `.KA1` | 963 | 254 – 5434 bytes (variable) |
| `.KAA` | 76 | **all exactly 134 660 bytes** |
| `.SYX` | 42 | mixed |
| `.KCA` | 13 | — |
| `.KRA` | 5 | — |
| `.IMG` | 15 | 1 474 560 bytes = exactly 1.44 MB |

The uniform KAA size and the exact 1 474 560-byte images are useful invariants for
early validation.

---

## KA1 — single preset

**Parsing: `Observed`.** First evidence pass over `Africa.KA1` (1866 B),
`CybaBars.KA1` (1060 B), `BassTalk.KA1` (1952 B), `BellWing.KA1` (1952 B),
`Century.KA1` (1866 B), `Choruz.KA1` (1866 B).

### Established

| Offset | Size | Field | Confidence | Evidence |
| --- | --- | --- | --- | --- |
| `0x28` | 8 | Preset name, ASCII, space-padded | Observed | `"Africa  "`, `"CybaBars"`, `"BassTalk"` at the same offset in every sample |

### Observed but not yet explained

- `0x00`–`0x27` is a fixed-size header block. Byte `0x00` differs per file
  (`0x69`, `0x66`, `0x04`) — plausibly a checksum, **not confirmed**.
- `0x20`–`0x27` is consistently in the `0x3D`–`0x44` range with a `0x00`
  terminator at `0x27`, immediately before the name. Function unknown.
- `0x30`–`0x5F` shows a highly repetitive pattern across all samples
  (`… 00 00 02 03 00 02 00 40 02 00 40 00 …`, a run of `0x40` bytes at
  `0x46`–`0x4D`). Consistent with per-source default/offset tables; unverified.
- File size varies with content, so the source count and additive/PCM composition
  must be derivable from the header. The rule is **not yet known**.

### Open questions

- Exact size and meaning of the header block.
- Is byte `0x00` a checksum? Over which range, and with what algorithm?
- How are ADD vs. PCM sources encoded and counted?
- Which bytes are reserved and must be preserved untouched on rename?
- Precise name character set and padding rule (space vs. `0x00`) — needed before
  any rename operation is implemented.

---

## KAA — single bank

**Parsing: `Observed`.** Evidence: `ABANKINT.KAA` and `lead3.kaa`, plus the size
census over all 76 reference banks.

### Established

- **Every** reference KAA is exactly **134 660 bytes** (`0x20E04`). A fixed
  container size, not a function of content.
- The file begins with a table of **4-byte big-endian** values, zero meaning
  *empty slot*. First entries of `ABANKINT.KAA`:

  ```
  0x0000: 00 34 4E 70   slot 0  → 0x00344E70
  0x0004: 00 34 4F C4   slot 1  → 0x00344FC4
  0x0008: 00 34 52 EA   slot 2  → 0x003452EA
  0x000C: 00 00 00 00   empty
  …
  ```

- The pointer values (≈ `0x344E70`) are **far larger than the file**, so they are
  **not** file offsets. They look like addresses in the instrument's memory map;
  the file offset is presumably `pointer − base`. The base has not been
  determined.
- Consecutive deltas match plausible single sizes: `0x344FC4 − 0x344E70 = 0x154`
  (340 B), `0x3452EA − 0x344FC4 = 0x326` (806 B).
- The table continues past `0x200` with the same structure, consistent with 128
  slots × 4 bytes = 512 bytes of pointers.

### Open questions

- Is the pointer table exactly 512 bytes (128 slots), or larger?
- What is the base address to convert pointers to file offsets?
- Where does preset data start, and is there a header between table and data?
- Are stored singles byte-identical to the corresponding KA1 payload, or is the
  KA1 file a wrapper around the same body? **This determines whether
  `KAA → KA1` can be a byte-exact extraction** (specification §5).
- How is free space / bank capacity encoded (specification §8)? The fixed file
  size suggests a fixed data region with a used/free watermark.
- Endianness must be confirmed as big-endian by a documented source, not only by
  plausibility.

---

## Single SysEx

**Parsing: `Unsupported`.** Not yet examined. Must establish manufacturer ID, model ID,
message framing, length encoding, bank/slot metadata and checksum before any
claim of support (specification §30).

## KC1 / KCA — multi

**Parsing: `Unsupported`.** Not yet examined. Scope decision pending — see
[`OPEN_QUESTIONS.md`](OPEN_QUESTIONS.md).

## FAT12 / 1.44 MB IMG

**Parsing: `Experimental`.** Standard IBM-PC 1.44 MB layout: 512-byte sectors, 2 heads,
80 cylinders, 18 sectors/track, 2 FAT copies, 224 root directory entries,
1 sector/cluster. Total 2880 sectors = 1 474 560 bytes, matching every reference
image exactly.

This is the only V1 format with a well-established public specification, so the
risk here is implementation correctness (12-bit cluster packing, chain traversal,
both FAT copies staying in sync), not format discovery. Generated images will be
cross-validated with an independent FAT tool (specification §31).

K5000-specific filename constraints (8.3, permitted characters) still need
confirmation against the hardware.
