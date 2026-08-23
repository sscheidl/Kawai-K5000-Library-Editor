# Format notes

Specification §6/§30: never claim support for an unverified variant, and never
guess undocumented binary fields silently.

## Two separate scales

Research evidence and implementation status are deliberately **not** the same
thing, and are never merged into one column. Understanding a field is not the
same as having shipped a tested parser for it.

### 1. Evidence label — what we know about a fact

Applies to the format facts in this document. Every field table below carries
one per row.

| Label | Meaning |
| --- | --- |
| `Hypothesis` | Proposed from documentation or inference; not yet tested against files. |
| `Observed` | Seen consistently in reference files, by inspection rather than by a systematic check. |
| `VerifiedAgainstCorpus` | Checked programmatically across the whole private corpus by a reproducible probe in `tools/research/`. |
| `GoldenTested` | Additionally asserted by an automated test in this repository. |
| `HardwareVerified` | Confirmed on the real K5000S / Gotek and logged in `HARDWARE_ACCEPTANCE.md`. |

Nothing in this document is `HardwareVerified` yet. The golden tests are
corpus-gated: they skip in public CI, where no corpus exists, and run locally
when `K5000_REFERENCE_CORPUS` is configured.

### 2. `VerificationLevel` — what the code actually does

Decided in [`OPEN_QUESTIONS.md` Q4](OPEN_QUESTIONS.md), anchored in the core as
`k5000::VerificationLevel`, tracked **separately for Parsing, Writing and
Conversion**:

```
Unsupported → Experimental → Observed → GoldenTested → HardwareVerified
```

Write paths below `GoldenTested` are disabled unless explicitly enabled, which
`isSupportedForWriting()` enforces and `tests/unit/test_verification.cpp`
asserts. A level is raised only in the commit that adds the code and tests
behind it.

## Per-format status

| Format | V1 scope | Evidence | Parsing | Writing | Conversion |
| --- | --- | --- | --- | --- | --- |
| KA1 (single) | required | `GoldenTested` | `GoldenTested` | `Unsupported` | `Unsupported` |
| KAA (single bank) | required | `GoldenTested` | `GoldenTested` | `Unsupported` | `Unsupported` |
| KAA → KA1 extraction | required | `GoldenTested` | n/a | n/a | byte-exact copy, reported under parsing |
| Single SysEx | required | `Hypothesis` | `Unsupported` | `Unsupported` | `Unsupported` |
| Bank SysEx | only if verifiable | none | `Unsupported` | `Unsupported` | `Unsupported` |
| KC1 / KCA (multi) | required, after Single/IMG core | none | `Unsupported` | `Unsupported` | `Unsupported` |
| Multi SysEx | only if verifiable | none | `Unsupported` | `Unsupported` | `Unsupported` |
| KRA (arpeggio) | pass-through only | none | `Unsupported` | `Unsupported` | n/a |
| FAT12 / 1.44 MB IMG | required | `Observed` | `Unsupported` | `Unsupported` | n/a |
| Bank capacity model | required | `VerifiedAgainstCorpus` | `Unsupported` | n/a | n/a |

Writing is `Unsupported` everywhere on purpose. See
[Stop conditions](#stop-conditions).

## Sources

- **"Making Sense of Kawai K5000 Patch Data Files"** — digitalsynth.net
  (Conifer Productions Oy, © 2019–2025). Local PDF, 5 pages. Reproduces a full
  `kaanalyz -lp` listing of `WIZOO.KAA`: base address, per-patch sizes,
  source-composition strings, memory-usage percentages.
- **Kawai MIDI implementation**, via the same lineage — the Single Tone checksum
  definition tested below.
- **`kaanalyz` by Jens Groh** (via the article) — the original C implementation
  behind the KAA pointer-table model. Not read directly.
- Kawai K5000 user manual, page 119 — the file-type list.

Still to consult for SysEx: the Kawai MIDI implementation document itself,
Edisyn's K5000 support, KSynthLib / k5ktool. Where two sources disagree, the
disagreement is recorded rather than resolved by preference.

## Reference corpus

Private, read-only, never committed ([`OPEN_QUESTIONS.md` Q2](OPEN_QUESTIONS.md)).
Public tests use synthetic fixtures built in code. Metadata is published in
[`../testdata/REFERENCE_MANIFEST.csv`](../testdata/REFERENCE_MANIFEST.csv) under
anonymous fixture ids — no payloads, no original filenames.

| Type | Count | Sizes |
| --- | --- | --- |
| `.KA1` | 963 | 254 – 5434 bytes |
| `.KAA` | 76 | all exactly 134 660 bytes |
| `.SYX` | 42 | mixed |
| `.KCA` | 13 | — |
| `.KRA` | 5 | — |
| `.IMG` | 15 | all exactly 1 474 560 bytes |

---

# KA1

## Known structure

A K5000 single patch is one contiguous byte range. A `.KA1` file is exactly that
range, with no header, wrapper or trailer.

| Part | Size | Count | Evidence |
| --- | --- | --- | --- |
| Common block | 82 bytes | 1 | `GoldenTested` |
| Source descriptor | 86 bytes | `S` = 1..6 | `GoldenTested` |
| ADD wave kit | 806 bytes | `A`, one per additive source | `GoldenTested` |

```
patch size = 82 + 86 * S + 806 * A
```

Source descriptors come first as a block, then the ADD wave kits in source
order. A PCM source contributes a descriptor but no wave kit.

The 82-byte common block is the checksum byte plus 81 bytes of common data. The
research literature quotes "81 bytes"; both descriptions refer to the same
layout and the constant in the code is the full 82-byte block including the
checksum.

**The entire payload is 7-bit.** No byte ≥ 0x80 occurs in any of the 937
structurally complete corpus files. This corroborates the claim that KAA patches
are in the same format as the K5000 SysEx data, and means a KA1 → SysEx
conversion should be framing only, with no nibbleization.

Reproduce: `python tools/research/ka1_source_probe.py <corpus>`

## Field table

| Field | Offset | Size | Meaning | Evidence |
| --- | --- | --- | --- | --- |
| checksum | `0x00` | 1 | Kawai Single Tone checksum, 7-bit | `GoldenTested` |
| *(unidentified)* | `0x01`–`0x26` | 38 | not analysed; preserved verbatim | — |
| constant | `0x27` | 1 | always `0x00`; the only byte constant across all 937 complete files | `VerifiedAgainstCorpus` |
| name | `0x28` | 8 | ASCII, space-padded; `0x7F` occurs as a K5000 display glyph | `GoldenTested` |
| *(unidentified)* | `0x30`–`0x32` | 3 | not analysed; preserved verbatim | — |
| source count `S` | `0x33` | 1 | 1..6 | `GoldenTested` |
| *(unidentified)* | `0x34`–`0x51` | 30 | not analysed; preserved verbatim | — |
| sources start | `0x52` | 86 × `S` | source descriptors | `GoldenTested` |
| wave kit number | source `+28` | 2 | two 7-bit bytes, high first; `512` = additive | `GoldenTested` |
| ADD kits start | `0x52 + 86·S` | 806 × `A` | additive wave kits | `GoldenTested` |
| ADD kit checksum | kit `+0` | 1 | same formula, over the kit | `VerifiedAgainstCorpus` |

Everything marked *unidentified* is carried through untouched. Not understanding
a byte is never a reason to normalize it (§5).

## Checksum

**`VerifiedAgainstCorpus`, and `GoldenTested`.**

```
checksum = ( sum(common[1..81]) + sum(all active source descriptors) + 0xA5 ) & 0x7F
```

The scope is the important part: the **ADD wave kits are excluded**. Each kit
carries its own checksum in its first byte, computed the same way over the rest
of the kit.

| Measurement | Result |
| --- | --- |
| Structurally complete files tested | 937 |
| Patch checksum matches | **937** |
| Patch checksum mismatches | **0** |
| Malformed / skipped | 26 |
| ADD wave kits tested | 1674 |
| Kit checksum matches | 1670 |
| Kit checksum mismatches | 4 |

The four kit failures are independent evidence for the scope rule: those files
have corrupt wave kit data and *still* pass the patch checksum, which can only
happen if the patch checksum does not cover the kits.

Eight earlier candidate formulas were tested and all failed; that negative
result is kept in the project history so the work is not repeated. This ninth
formula was not invented — it comes from the documented Kawai definition and was
then tested, not assumed.

Reproduce: `python tools/research/ka1_checksum_probe.py <corpus>`

## ADD versus PCM detection

**`VerifiedAgainstCorpus`, and `GoldenTested`.**

Each 86-byte source descriptor carries a wave kit number at source-relative
offset `+28/+29`, as two 7-bit bytes, high byte first:

```
wave = (descriptor[+28] << 7) | descriptor[+29]
additive  <=>  wave == 512
```

Cross-checked against the file length, which fixes `A` independently:

| Measurement | Result |
| --- | --- |
| Files analysed | 938 |
| Sources inspected | 2611 |
| ADD sources | 1676 |
| PCM sources | 935 |
| Mixed ADD/PCM patches | 451 |
| Structural contradictions | **1** |
| `(S, A)` combinations observed | 24 |

The single contradiction is catalogued below. Non-additive wave numbers occupy
341..511 with one out-of-range outlier; the meaning of the PCM numbering is not
needed for V1 and is left open.

Reproduce: `python tools/research/ka1_source_probe.py <corpus>`

## Structural length calculation

This is the only permitted way to determine how long a patch is:

```
1. read the common block                  -> 82 bytes
2. read S from common offset 0x33         -> reject unless 1 <= S <= 6
3. read exactly S source descriptors      -> 86 * S bytes
4. classify each source by its wave kit   -> A = count of wave == 512
5. read exactly A ADD wave kits           -> 806 * A bytes
6. the patch ends there
```

Every step is bounds-checked against the input span before it reads. See
[`src/formats/ka1/Parser.cpp`](../src/formats/ka1/Parser.cpp).

## Malformed variants in the corpus

25 of 963 files fail the structural traversal, and 1 more is ambiguous. All are
**reported, never repaired** (§5). They are catalogued by expected parse state
in [`REFERENCE_MANIFEST.csv`](../testdata/REFERENCE_MANIFEST.csv).

| Category | Count | Behaviour |
| --- | --- | --- |
| Trailing bytes after a complete patch (`+2`, `+6`, `+10`, …) | most of the 25 | `Malformed`, payload truncated to the structural extent, extra bytes never adopted |
| Source count out of range (`0`, `127`) | 2 | `Malformed`, rejected before reading further |
| No valid `(S, A)` combination for the file length | 2 | `Malformed` |
| Length and descriptors disagree on `A` | 1 | `StructurallyAmbiguous` |
| Corrupt ADD wave kit data | 4 | `ChecksumMismatch`, patch checksum still valid |

The ambiguous file deserves note: its length implies three additive sources
while its descriptors imply two, and its **patch checksum is correct**. It is
self-consistent and still wrong, which is exactly the case that a "looks fine,
ship it" reader would mis-extract. The parser refuses to choose.

## Validation states

`Valid` · `ChecksumMismatch` · `UnsupportedVariant` · `StructurallyAmbiguous` ·
`Truncated` · `Malformed`

Ordered by how unusable the payload is; the worst observation wins when several
combine. An ambiguous extent outranks an unrecognised variant, because not
knowing which bytes belong to the patch is worse than not knowing what they
mean. Reaching the end of the input is never by itself a reason to report
`Valid`.

---

# KAA

## Fixed file size

**`GoldenTested`.** Every one of the 76 reference banks is exactly
**134 660 bytes**.

| Region | Offset | Size |
| --- | --- | --- |
| Pointer table | `0x0000` | 3584 bytes = 128 slots × 7 dwords |
| End-of-used-data pointer | `0x0E00` | 4 bytes |
| Patch data region | `0x0E04` | 131 072 bytes (`0x20000`) |
| **Total** | | **134 660 bytes** |

`0xE04 + 0x20000 = 134660`. The 131 072-byte region is independently confirmed
by the published `kaanalyz` listing, which reports 87 424 bytes used as
"66.70% of memory" — 87 424 / 131 072 = 66.70 %.

## Pointer table

**`GoldenTested`.** Pointers are 4-byte **big-endian absolute addresses in the
instrument's memory map**, not file offsets. Zero means absent.

Each slot owns 7 consecutive pointers:

| Index | Meaning |
| --- | --- |
| 0 | Start of the patch. Zero ⇒ empty slot. |
| 1–6 | ADD wave kit for source 1–6. Diagnostic only — see the prohibition below. |

## Pointer interpretation

```
base        = smallest non-zero pointer anywhere in the table
file_offset = 0x0E04 + (pointer - base)
```

The base is **bank-dependent, not a constant**: `WIZOO.KAA` reports
`0x00344E74`, `ABANKINT.KAA` resolves to `0x00344E70`, `lead3.kaa` to
`0x0032407C`. It is recovered from the table, never assumed.

A base chosen because it makes one file work would prove nothing, so the mapping
was validated the hard way — by parsing the patch at every computed offset and
checking its Kawai checksum. A wrong base shifts every patch and destroys every
checksum.

| Measurement | Result |
| --- | --- |
| Banks tested | 76 |
| Banks where the base equals the lowest patch pointer | 76 / 76 |
| Occupied slots | 4072 |
| Patch checksum OK at the computed offset | **4072 / 4072** |
| Structurally rejected | 0 |
| Offsets outside the data region | 0 |
| Data-region usage | 0.8 % – 99.8 % |

The trailing dword at `0x0E00` is the end-of-used-data pointer:
`used = tail - base`.

Reproduce: `python tools/research/kaa_pointer_probe.py <corpus>`

## Capacity

**`VerifiedAgainstCorpus`.** A bank is bounded twice:

1. **128 slots** — the pointer table holds no more.
2. **131 072 bytes** of patch data — the fixed data region.

The byte budget binds first in practice: `ABANKINT.KAA` reaches 99.5 % of it
with only 60 patches. Per [Q3](OPEN_QUESTIONS.md), exceeding it produces a
warning rather than a hard export gate until the model is `GoldenTested` end to
end and confirmed on hardware.

## Fragmentation and padding

**`GoldenTested`.** Patch data is mostly contiguous but not always. 8 of the 76
corpus banks contain gaps where a patch does not abut its neighbour;
`ABANKINT.KAA` has 8 such gaps among its 60 occupied slots.

## Stale ADD-kit pointers

**`GoldenTested`.** 28 slots across the corpus carry an ADD wave kit pointer for
a source that does not exist — for example a pointer for source 4 in a patch
that has two sources. These are leftovers from a larger patch that previously
occupied the slot. In 29 slots the pointer-derived ADD count disagrees with the
wave kit classification.

Sizing from the pointer count instead of the descriptors produces measurably
worse results: extraction reproduces 2210 standalone KA1 files byte-for-byte
when sized structurally, and only 2195 when sized from the pointer table.

## Extraction algorithm

```
parse the pointer table
derive base = smallest non-zero pointer
for each occupied slot:
    offset = 0x0E04 + (pointer - base)
    reject if offset falls outside the data region
    parse the patch structurally, bounded by the rest of the file
    length = 82 + 86*S + 806*A          <- from the patch, nothing else
    copy exactly those bytes
```

Failures are reported per slot and never silently dropped.

## Prohibition on pointer-derived sizing

**Invariant A. Both of these are forbidden:**

```
size = next_pointer - pointer                  // banks are fragmented
size = f(count of non-zero ADD kit pointers)   // entries go stale
```

Also forbidden: `size = bytes remaining until the next occupied slot`.

Pointers may locate patch **starts**. Patch **length** comes exclusively from
the patch's own structure. A reader that violates this silently welds padding,
or a phantom 806-byte wave kit, onto the end of an extracted payload — and the
result still looks plausible, which is what makes it dangerous.

Two permanent regression tests cover this, in
[`tests/unit/test_kaa_bank.cpp`](../tests/unit/test_kaa_bank.cpp):

- `INVARIANT A: patch length never comes from the pointer delta` — builds a bank
  with a 96-byte gap and asserts the extracted payload is the structural length,
  not the pointer delta.
- `INVARIANT A: a stale ADD-kit pointer does not lengthen the patch` — builds a
  two-source PCM patch with a stale kit pointer and asserts the payload stays
  254 bytes.

Both would fail if pointer-delta sizing were reintroduced.

## Preservation

**`GoldenTested`.** Extraction is byte preservation, not serialization. Nothing
is normalized, recomputed, reordered or renamed; a patch with a wrong stored
checksum is extracted with that wrong checksum intact.

Against the corpus: 2210 extracted patches are byte-identical to a standalone
KA1 of the same name and size. 197 same-name/same-size pairs genuinely differ —
independent edits sharing a name, not extraction errors.

---

# Single SysEx

**`Hypothesis`.** The patch payload is entirely 7-bit and the digitalsynth
source states that KAA patches are in the same format as the SysEx files, so a
Single SysEx message is expected to carry the same payload inside standard
framing.

Nothing is verified. Manufacturer ID, model ID, framing, length encoding,
bank/slot metadata and the checksum's placement in the message all remain
unexamined. The corpus has 42 `.SYX` files to check against. No support is
claimed (§30).

# KC1 / KCA — multi

Not examined. Sequenced after the Single/IMG core by
[decision Q1](OPEN_QUESTIONS.md).

# FAT12 / 1.44 MB IMG

**`Observed`.** Standard IBM-PC 1.44 MB layout: 512-byte sectors, 2 heads,
80 cylinders, 18 sectors/track, 2 FAT copies, 224 root directory entries,
1 sector/cluster. Total 2880 sectors = 1 474 560 bytes, matching all 15
reference images exactly.

The risk here is implementation correctness — 12-bit cluster packing, chain
traversal, keeping both FAT copies in sync — not format discovery. Generated
images will be cross-validated with an independent FAT tool (§31).

---

# Stop conditions

Currently in force:

- **No mutating KA1 writer.** The checksum rule is now verified, but no
  serializer exists and no round-trip test covers one. Writing stays
  `Unsupported`, which `tests/unit/test_verification.cpp` asserts.
- **No KAA writer or rebuilder.** Patch boundaries are proven for reading;
  allocation, padding semantics and whether the instrument requires
  address-ordered patches are not.
- **No bank or Multi SysEx conversion.** Field similarity is not evidence.
- **Nothing is `HardwareVerified`.** No generated file has been loaded on a real
  K5000S.

# Remaining open questions

**KA1**

- Meaning of common-data bytes other than `0x27`, `0x28`–`0x2F` and `0x33`.
- Internal layout of the 86-byte source descriptor beyond the wave kit number.
- Internal layout of the 806-byte ADD wave kit. Not needed for V1 — it is
  carried verbatim — but needed to validate rather than merely checksum it.
- Which bytes are reserved and must survive a rename untouched. **This blocks
  rename**, which is otherwise the simplest possible mutation.
- Name character set and padding rule. Space padding is observed; whether `0x00`
  padding is accepted by the hardware is unknown, and `0x7F` glyphs must be
  preserved rather than sanitized.
- The PCM wave kit numbering (observed range 341..511) is unexplained. Not
  needed for V1: only the `== 512` test matters.

**KAA**

- Is the base address stored explicitly anywhere, or is "smallest non-zero
  pointer" the rule the instrument itself uses?
- What occupies the gaps in a fragmented bank — stale data or defined padding?
  It must be preserved either way.
- Does the instrument require patches to be sorted by address? The published
  `WIZOO.KAA` listing shows slots badly out of address order, which suggests
  not, but this must be settled **before any bank is rebuilt**.
- Does anything mark a bank as "A" or "D", or is that purely the load
  destination?
