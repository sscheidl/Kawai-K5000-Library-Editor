# Format notes

Working document for Phase A. **Nothing here may be promoted to implementation
while it is still marked _Observed_ or _Unknown_.** Specification §6/§30: never
claim support for an unverified variant, and never guess undocumented binary
fields silently.

## Confidence levels

| Level | Meaning |
| --- | --- |
| **Verified** | Confirmed by documentation *and* reproduced across ≥ 2 independent reference files, with a round-trip test. |
| **Documented** | Described by a credible source, not yet reproduced against reference files here. |
| **Observed** | Seen consistently in reference files, no documentary confirmation yet. |
| **Unknown** | Not understood. Bytes are preserved verbatim; no semantic claim is made. |

## Per-format status

| Format | V1 scope | Status | Notes |
| --- | --- | --- | --- |
| KA1 (single) | required | Observed | Header + 8-char name located; body layout open |
| KAA (single bank) | required | Observed | 4-byte big-endian pointer table located |
| Single SysEx | required | Unknown | Framing not yet examined |
| Bank SysEx | only if verifiable | Unknown | May be dropped from V1 |
| KC1 (multi) | required | Unknown | Not yet examined |
| KCA (multi bank) | required | Unknown | Not yet examined |
| Multi SysEx | only if verifiable | Unknown | May be dropped from V1 |
| KRA (arpeggio) | pass-through only | Unknown | Stored in images, never parsed in V1 |
| FAT12 / 1.44 MB IMG | required | Documented | Standard IBM-PC FAT12; industry-documented |

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

Local, read-only. See [`TEST_MATRIX.md`](TEST_MATRIX.md) for how it is used.

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

**Status: Observed.** First evidence pass over `Africa.KA1` (1866 B),
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

**Status: Observed.** Evidence: `ABANKINT.KAA` and `lead3.kaa`, plus the size
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

**Status: Unknown.** Not yet examined. Must establish manufacturer ID, model ID,
message framing, length encoding, bank/slot metadata and checksum before any
claim of support (specification §30).

## KC1 / KCA — multi

**Status: Unknown.** Not yet examined. Scope decision pending — see
[`OPEN_QUESTIONS.md`](OPEN_QUESTIONS.md).

## FAT12 / 1.44 MB IMG

**Status: Documented.** Standard IBM-PC 1.44 MB layout: 512-byte sectors, 2 heads,
80 cylinders, 18 sectors/track, 2 FAT copies, 224 root directory entries,
1 sector/cluster. Total 2880 sectors = 1 474 560 bytes, matching every reference
image exactly.

This is the only V1 format with a well-established public specification, so the
risk here is implementation correctness (12-bit cluster packing, chain traversal,
both FAT copies staying in sync), not format discovery. Generated images will be
cross-validated with an independent FAT tool (specification §31).

K5000-specific filename constraints (8.3, permitted characters) still need
confirmation against the hardware.
