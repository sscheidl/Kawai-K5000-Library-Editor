# Format notes

Working document for Phase A. Specification §6/§30: never claim support for an
unverified variant, and never guess undocumented binary fields silently.

## Two separate scales

Research evidence and implementation status are deliberately **not** the same
thing, and are never merged into one column. Understanding a field is not the
same as having shipped a tested parser for it.

### 1. Research evidence — what we know

Applies to this document only. It records how well a structure is understood and
governs what may legitimately be implemented next.

| Level | Meaning |
| --- | --- |
| `None` | Not examined. |
| `Observed` | Seen consistently across reference files; no documentary confirmation. |
| `Documented` | Described by a credible source; not yet reproduced against reference files here. |
| `Corroborated` | Documented **and** reproduced across independent reference files, with sources in agreement or their disagreement recorded. |

### 2. `VerificationLevel` — what the code actually does

Defined in [`OPEN_QUESTIONS.md` Q4](OPEN_QUESTIONS.md) (decided 2026-08-23),
anchored in the core, tracked **separately for Parsing, Writing and
Conversion**:

```
Unsupported → Experimental → Observed → GoldenTested → HardwareVerified
```

| Level | Meaning |
| --- | --- |
| `Unsupported` | Not implemented. No semantic claim; bytes are preserved verbatim. |
| `Experimental` | Implemented, not yet reproduced against reference files. Writing off by default. |
| `Observed` | Implemented and reproduced consistently across reference files. |
| `GoldenTested` | Round-trip and golden-file tests pass against the reference corpus. |
| `HardwareVerified` | Confirmed on the real K5000S / Gotek and logged in `HARDWARE_ACCEPTANCE.md`. |

The axes move independently — parsing routinely leads writing, and a conversion
is capped by the weaker of its two endpoints. Write paths below `GoldenTested`
are disabled unless the user explicitly enables them, and every such export is
logged with its level.

**Because no parser exists yet, every implementation level below is
`Unsupported`, regardless of how much research evidence has accumulated.** A
level is raised in the same commit that adds the code and tests backing it,
never in advance.

## Per-format status

| Format | V1 scope | Research evidence | Parsing | Writing | Conversion |
| --- | --- | --- | --- | --- | --- |
| KA1 (single) | required | `Corroborated` | `Unsupported` | `Unsupported` | `Unsupported` |
| KAA (single bank) | required | `Corroborated` | `Unsupported` | `Unsupported` | `Unsupported` |
| Single SysEx | required | `Observed` | `Unsupported` | `Unsupported` | `Unsupported` |
| Bank SysEx | only if verifiable | `None` | `Unsupported` | `Unsupported` | `Unsupported` |
| KC1 (multi) | required, after Single/IMG core | `None` | `Unsupported` | `Unsupported` | `Unsupported` |
| KCA (multi bank) | required, after Single/IMG core | `None` | `Unsupported` | `Unsupported` | `Unsupported` |
| Multi SysEx | only if verifiable | `None` | `Unsupported` | `Unsupported` | `Unsupported` |
| KRA (arpeggio) | pass-through only | `None` | `Unsupported` | `Unsupported` | n/a |
| FAT12 / 1.44 MB IMG | required | `Documented` | `Unsupported` | `Unsupported` | n/a |
| Bank capacity model | required | `Corroborated` | `Unsupported` | n/a | n/a |

Notes per row:

- **KA1 / KAA** — container, pointer table, patch layout and size formula are
  documented *and* reproduced across the corpus. Two things remain open: the
  checksum algorithm and the meaning of most common-data bytes.
- **Single SysEx** — the KA1 payload is 7-bit clean and, per the digitalsynth
  source, is the same data the SysEx message carries. Framing itself is still
  unexamined.
- **KC1 / KCA** — not yet examined; sequenced after the Single/IMG core (Q1).
- **KRA** — stored in images byte-exact, never parsed in V1.
- **FAT12** — publicly specified; nothing implemented yet.
- **Bank capacity** — 128 slots *and* a 131 072-byte patch-data budget; see the
  KAA section. Per [Q3](OPEN_QUESTIONS.md) it stays a warning until the
  implementation reaches `GoldenTested`.

## Sources

Primary documentation used so far:

- **"Making Sense of Kawai K5000 Patch Data Files"** — digitalsynth.net
  (Conifer Productions Oy, © 2019–2025). Local PDF, 5 pages. Describes the
  historical utility lineage and reproduces a full `kaanalyz -lp` listing of
  `WIZOO.KAA`, which is the key quantitative source: base address, per-patch
  sizes, source-composition strings and memory-usage percentages.
- **`kaanalyz` by Jens Groh** (via the article) — the original C implementation
  that established the KAA pointer-table model. Not read directly; its behaviour
  is inferred from the published listing and the article's description.
- Kawai K5000 user manual, page 119 — the file-type list (`.KAA` bank of
  singles, `.KA1` single, `.KCA` bank of multi/combi, `.KC1` single multi/combi,
  `.KRA` arpeggiator).

Still to consult: the Kawai MIDI implementation document (SysEx framing and the
checksum), the two local Wizoo PDFs, Edisyn's K5000 support, and
KSynthLib / k5ktool. Where two sources disagree, the disagreement is recorded
rather than resolved by preference.

## Reference corpus

Private local golden corpus (OPEN_QUESTIONS Q2): **read-only, never committed,
never modified by tests**. Public tests use synthetic fixtures. See
[`TEST_MATRIX.md`](TEST_MATRIX.md) for how it is used.

| Type | Count | Sizes |
| --- | --- | --- |
| `.KA1` | 963 | 254 – 5434 bytes (variable) |
| `.KAA` | 76 | **all exactly 134 660 bytes** |
| `.SYX` | 42 | mixed |
| `.KCA` | 13 | — |
| `.KRA` | 5 | — |
| `.IMG` | 15 | 1 474 560 bytes = exactly 1.44 MB |

Both the uniform KAA size and the exact 1 474 560-byte images are now explained
and make good early validation invariants.

---

## Patch payload — shared by KA1 and KAA

**Research evidence: `Corroborated`. Implementation: `Unsupported`.**

A K5000 single patch is one contiguous byte range:

| Part | Size | Count |
| --- | --- | --- |
| Common data | 82 bytes | 1 |
| Source descriptor | 86 bytes | `S` = number of sources (1–6) |
| ADD wave kit | 806 bytes | `A` = number of additive sources (0 ≤ `A` ≤ `S`) |

```
patch size = 82 + 86 * S + 806 * A
```

Source descriptors come first as a block, then the ADD wave kits in source
order. A PCM source contributes a descriptor but no wave kit.

### Evidence

Derived from the ten distinct sizes in the article's `WIZOO.KAA` listing, each
annotated with a source-composition string such as `AP----` or `AAPP--`. The
system is over-determined and solves exactly:

| Composition | Listed size | `82 + 86·S + 806·A` |
| --- | --- | --- |
| `PP----` | 254 | 82 + 172 + 0 |
| `AP----` | 1060 | 82 + 172 + 806 |
| `APP---` | 1146 | 82 + 258 + 806 |
| `APPP--` | 1232 | 82 + 344 + 806 |
| `AA----` | 1866 | 82 + 172 + 1612 |
| `AAP---` | 1952 | 82 + 258 + 1612 |
| `AAPP--` | 2038 | 82 + 344 + 1612 |
| `APPAP-` | 2124 | 82 + 430 + 1612 |
| `AAA---` | 2758 | 82 + 258 + 2418 |
| `AAAP--` | 2844 | 82 + 344 + 2418 |

Reproduced independently against the corpus: **938 of 963 KA1 files** satisfy
the formula with `S` read from common offset `0x33`. The observed `(S, A)`
distribution covers all 24 combinations that occur, from `(2,0)` to `(6,6)`, and
the largest, `(6,6)` = 5434 bytes, is exactly the largest KA1 in the corpus.

### Established common-data fields

| Offset | Size | Field | Evidence |
| --- | --- | --- | --- |
| `0x00` | 1 | Checksum (presumed) | Always 7-bit; all 128 values occur across the corpus. Algorithm **not determined** — see below. |
| `0x27` | 1 | Constant `0x00` | The only byte constant across all 938 valid files. Immediately precedes the name. |
| `0x28` | 8 | Preset name, ASCII, space-padded | Decodes correctly at this offset in every corpus file and at every computed KAA slot offset. `0x7F` occurs inside names (a K5000 display glyph), so the name is not plain printable ASCII. |
| `0x33` | 1 | Number of sources `S` (1–6) | The **only** byte in `0x30`–`0x51` that equals the independently derived source count for every patch in both banks analysed. |

**The entire patch payload is 7-bit: none of the 938 structurally valid KA1
files contains a byte ≥ 0x80.** This corroborates the article's statement that
KAA patches are in the same format as the K5000 SysEx files — a SysEx body
cannot carry 8-bit data, so no nibbleization or bit-packing is involved and
KA1 → SysEx should be pure framing.

### KA1 is the patch payload verbatim

**A `.KA1` file is byte-for-byte the patch payload, with no header, wrapper or
trailer.**

Evidence: extracting every patch from every KAA bank in the corpus and matching
against KA1 files of the same name and length gives **2195 byte-identical
pairs**, 190 same-name/same-length pairs that genuinely differ (independent
edits sharing a name), and 1687 patches with no counterpart file.

Consequence for specification §5: **`KAA → KA1` can be a byte-exact
extraction**, and `KA1 → KAA` a byte-exact insertion plus pointer maintenance.
Nothing is re-serialized on that path, so nothing can be silently altered on it.
This is the most important finding so far and the tests must pin it.

### Checksum — unresolved

Byte `0x00` is presumed to be a checksum. It is always 7-bit and takes all 128
values across the corpus, consistent with that reading but not proof of it. The
following candidates were tested against all 938 valid files and **all failed**
(hits out of 938):

| Candidate | Hits |
| --- | --- |
| `(sum(payload[1:]) + 0xA5) & 0x7F` | 133 |
| `sum(payload[2:82]) & 0x7F` | 12 |
| `sum(payload[1:82]) & 0x7F` | 10 |
| `xor(payload[1:])` | 6 |
| `(sum(payload[1:82]) + 0xA5) & 0x7F` | 5 |
| `sum(payload[1:]) & 0x7F` | 4 |
| `(-sum(payload[1:])) & 0x7F` | 3 |
| `sum(payload[1:]) & 0xFF` | 2 |

Recorded as a negative result so the work is not repeated. The Kawai MIDI
implementation document is the next place to look; the checksum is likely
defined there for the SysEx message rather than for the file.

Until it is resolved, **writing is blocked on any path that would need to
recompute it** — which is every path except byte-exact copy. Byte-exact
extraction and insertion do not touch it, which is a further reason to build
those first.

### Remaining open questions

- Meaning of common-data bytes other than `0x27`, `0x28`–`0x2F` and `0x33`.
- Internal layout of the 86-byte source descriptor, in particular the flag that
  distinguishes ADD from PCM. `A` is currently derived from the KAA pointer
  table, which is unavailable when reading a bare KA1.
- Internal layout of the 806-byte ADD wave kit. Not needed for V1 — it is
  carried verbatim — but needed to validate it.
- Which bytes are reserved and must survive a rename untouched.
- Name character set and padding rule. Space padding is observed; whether `0x00`
  padding is also accepted by the hardware is unknown, and `0x7F` glyphs must be
  preserved rather than sanitized away.

### Malformed files in the corpus

25 of 963 KA1 files do not satisfy the size formula. They are useful as
real-world malformed-input fixtures (§27) and must be **reported, never
auto-repaired** (§5):

| File | Size | `byte[0x33]` | Note |
| --- | --- | --- | --- |
| `MiMoog1-3.KA1` | 1954 | 3 | 1952 + 2 trailing bytes |
| `MiMoog4.KA1` | 1962 | 3 | 1952 + 10 |
| `Nightrun.KA1` | 2764 | 3 | 2758 + 6 |
| `DX_Rhds1.KA1` | 2762 | 0 | source count 0 is out of range |
| `Horns_03.ka1` | 428 | 4 | 426 + 2 |
| `NightrEf.KA1` | 1871 | 127 | source count 127 — badly malformed |
| `Gambit.KA1` | 3663 | 4 | no valid `(S, A)` combination |
| `Glocke01.KA1` | 3655 | 4 | no valid `(S, A)` combination |

The recurring "+2 / +6 / +10 trailing bytes" pattern suggests a tool that
appended data on save rather than genuine corruption. Do not assume that; report
the discrepancy with offset and expected/actual size (§35).

---

## KAA — single bank

**Research evidence: `Corroborated`. Implementation: `Unsupported`.**

### Container layout

| Region | Offset | Size |
| --- | --- | --- |
| Pointer table | `0x000000` | 3584 bytes = 128 patches × 7 pointers × 4 |
| End-of-data pointer | `0x000E00` | 4 bytes |
| Patch data region | `0x000E04` | 131 072 bytes (`0x20000`) |
| **Total** | | **134 660 bytes** |

`0xE04 + 0x20000 = 134660` — exactly the size of every one of the 76 reference
banks. The 131 072-byte data region is confirmed independently by the article's
listing, which reports 87 424 bytes used as "66.70% of memory"
(87 424 / 131 072 = 66.70 %).

### Pointer table

Pointers are **4-byte big-endian absolute addresses in the instrument's memory
map**, not file offsets. Zero means absent.

Each patch owns **7 consecutive pointers**:

| Index | Meaning |
| --- | --- |
| 0 | Start of the patch (common data + source descriptors). Zero ⇒ empty slot. |
| 1–6 | ADD wave kit for source 1–6. Zero ⇒ that source is PCM or absent. |

The number of additive sources is therefore the count of non-zero pointers in
1..6, and the source count comes from common offset `0x33`. Together they give
the patch size through the formula above.

### Address base

```
base        = minimum non-zero pointer in the table
file_offset = 0xE04 + (pointer - base)
```

The base is **not a constant**. The article's `WIZOO.KAA` reports `0x00344E74`;
`ABANKINT.KAA` in the corpus resolves to `0x00344E70` and `lead3.kaa` to
`0x0032407C`. It is a property of the bank, recovered from the table rather than
assumed.

Validated by decoding the 8-byte name at `file_offset + 0x28` for every occupied
slot: **4072 patches across the corpus KAA banks** resolve to well-formed names
under this rule.

The trailing dword at `0x0E00` is the **end-of-used-data pointer**:
`used_bytes = tail - base`. For `ABANKINT.KAA` that is 130 408 of 131 072 bytes;
for `lead3.kaa`, 74 032.

### Capacity model (specification §8, decision Q3)

A bank is bounded by **two** independent limits:

1. **128 slots** — the pointer table has room for no more.
2. **131 072 bytes of patch data** — the fixed data region.

The byte budget is the binding constraint in practice: 128 average additive
patches would far exceed it. `ABANKINT.KAA` reaches 99.5 % of the byte budget
with only 60 patches, while `lead3.kaa` uses 56.5 % with 50.

This is enough to build the capacity model, but it is not `GoldenTested`, so per
[Q3](OPEN_QUESTIONS.md) exceeding it produces a warning rather than a hard
export gate until tests and hardware confirm it.

### Fragmentation and padding

Patch data is *mostly* contiguous but not always. In `ABANKINT.KAA`, 8 of 60
patches do not abut their neighbour, so gaps exist between allocations. The
article's listing carries a `padding` column (all zeros for `WIZOO.KAA`),
confirming that padding is a real, tracked property.

Consequence: patch size must be computed as `82 + 86·S + 806·A`, **never** from
the distance to the next pointer. A reader that infers size from pointer deltas
will silently read padding into the payload of a fragmented bank.

### Remaining open questions

- Is the base address stored anywhere explicitly, or is "minimum non-zero
  pointer" the rule the instrument itself uses? The three known values differ,
  so it is at least bank-dependent.
- What occupies the gaps in a fragmented bank — stale data, or defined padding?
  It must be preserved either way (§5).
- Does the instrument require patches to be sorted by address, or is slot order
  independent of data order? `WIZOO.KAA` lists slots whose addresses are clearly
  out of order (slot 60 at relative `0x000000`), which suggests independence —
  but that must be confirmed before a bank is ever rebuilt.
- Does anything in the file mark a bank as "A" or "D", or is that purely the
  destination chosen at load time?

---

## Single SysEx

**Research evidence: `Observed`. Implementation: `Unsupported`.**

The one established fact is indirect but useful: the patch payload is entirely
7-bit, and the digitalsynth source states that KAA patches are in the same
format as the SysEx files. A Single SysEx message is therefore expected to carry
the same payload inside standard framing.

Not yet examined, and required before any claim of support (§30): manufacturer
ID, model ID, message framing, length encoding, bank/slot metadata, and the
checksum — possibly the same unresolved algorithm as KA1 byte `0x00`.

The corpus has 42 `.SYX` files to check this against.

## Bank SysEx / Multi SysEx

**Research evidence: `None`. Implementation: `Unsupported`.** May be dropped
from V1 if they cannot be verified (§6).

## KC1 / KCA — multi

**Research evidence: `None`. Implementation: `Unsupported`.** Not yet examined;
sequenced after the Single/IMG core by [decision Q1](OPEN_QUESTIONS.md). The
corpus has 13 `.KCA` files and no loose `.KC1`.

## FAT12 / 1.44 MB IMG

**Research evidence: `Documented`. Implementation: `Unsupported`.**

Standard IBM-PC 1.44 MB layout: 512-byte sectors, 2 heads, 80 cylinders,
18 sectors/track, 2 FAT copies, 224 root directory entries, 1 sector/cluster.
Total 2880 sectors = 1 474 560 bytes, matching all 15 reference images exactly.

This is the only V1 format with a well-established public specification, so the
risk is implementation correctness — 12-bit cluster packing, chain traversal,
keeping both FAT copies in sync — not format discovery. Generated images will be
cross-validated with an independent FAT tool (§31).

K5000-specific filename constraints (8.3 form, permitted characters) still need
confirmation against the hardware.
