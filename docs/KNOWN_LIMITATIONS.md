# Known limitations

Kept honest and current. An empty section is better than an optimistic one.

## What works

Reading only, and only for two formats:

- **KA1** — parse, classify sources, verify the checksum, determine the exact
  patch length from the patch's own structure.
- **KAA** — parse the bank, map pointers to file offsets, extract occupied slots
  byte-for-byte.

Everything else is `Unsupported`.

## What does not work yet

**No writing of any kind.** There is no KA1 serializer, no KAA writer, no bank
rebuilder. `isSupportedForWriting()` returns false for every format and a unit
test asserts it. This is deliberate:

- **Rename is blocked** even though it looks trivial. Which common-data bytes
  are reserved is unknown, and the checksum would have to be recomputed over a
  block that is only partly understood.
- **Bank rebuilding is blocked.** Reading a bank is proven; the allocation rule,
  the meaning of the gaps in a fragmented bank, and whether the instrument
  requires address-ordered patches are all unknown.

**No SysEx.** Single SysEx is a hypothesis with no verified framing. Bank and
Multi SysEx have not been examined.

**No Multi (KC1/KCA).** Sequenced after the Single/IMG core by decision Q1.

**No FAT12/IMG.** The format is publicly documented; nothing is implemented.

**No GUI.** Qt 6 is not installed on the development machine, so the GUI target
is skipped by default. See [`TOOLCHAIN.md`](TOOLCHAIN.md).

## Structural limitations of Version 1

Out of scope by design (specification §22): full synthesis/additive editor,
automatic categorization, AI classification, cloud or online services, KRA
graphical editing, ME-1 E/F support, automatic duplicate cleanup, direct MIDI
communication, live audition, hardware discovery.

No deduplication subsystem (§21). Physical deletion of source files is
deliberately not implemented (§24); "remove from workspace" never touches the
file system.

## Verification honesty

Parsing for KA1 and KAA is `GoldenTested`, but the golden tests are
**corpus-gated**: they run only where `K5000_REFERENCE_CORPUS` points at an
extracted copy of the private corpus, and skip in public CI. A clean public
checkout therefore proves less than a local run does.

**Nothing is `HardwareVerified`.** No file produced or read by this project has
been loaded on a real K5000S. Until that happens, every claim here is about
bytes, not about the instrument.

## Format gaps that constrain the roadmap

Recorded in full in [`FORMAT_NOTES.md`](FORMAT_NOTES.md):

- Most of the 82-byte common block is unidentified and carried verbatim.
- The 806-byte ADD wave kit's internal layout is unknown; it is copied and
  checksummed, never interpreted.
- The PCM wave kit numbering (observed range 341..511) is unexplained. This does
  not affect V1, which only needs the `== 512` additive test.
- One corpus file is structurally ambiguous and is reported as such rather than
  guessed. 25 more are malformed and are rejected with a reason.
