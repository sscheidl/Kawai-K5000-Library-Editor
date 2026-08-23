#pragma once

// Synthetic fixtures.
//
// These are built in code rather than committed as binaries, so the public test
// suite needs no reference material at all (docs/OPEN_QUESTIONS.md Q2). They
// are constructed from the layout constants and given correct checksums, so a
// test that fails here is a parser bug, not a fixture problem.

#include "k5000/ka1/Layout.h"

#include <cstdint>
#include <string>
#include <vector>

namespace k5000::test {

struct SourceSpec {
    bool additive = false;
    std::uint16_t pcmWaveKit = 400; // ignored when additive
};

/// Build a structurally valid patch payload with a correct patch checksum and
/// correct per-kit checksums. Filler bytes stay 7-bit, like real payloads.
std::vector<std::uint8_t> makeSingle(const std::string& name,
                                     const std::vector<SourceSpec>& sources,
                                     std::uint8_t filler = 0x40);

/// Recompute and store the patch checksum in place. Used by tests that mutate
/// a fixture and still want it otherwise well-formed.
void refreshChecksum(std::vector<std::uint8_t>& payload);

/// One patch placed into a bank under construction.
struct BankEntry {
    int slot = 0;
    std::vector<std::uint8_t> payload;
    /// Bytes of dead space to leave *before* this patch. This is what makes a
    /// bank fragmented, and it is the whole point of the Invariant A test.
    std::size_t gapBefore = 0;
};

struct BuiltBank {
    std::vector<std::uint8_t> bytes;
    std::uint32_t baseAddress = 0;
    /// File offset of each entry, in the order given.
    std::vector<std::size_t> offsets;
};

/// Build a KAA bank of the exact real size, with a correct pointer table.
///
/// `staleKitPointerSlot`, when non-negative, additionally sets an ADD wave kit
/// pointer for a source that does not exist in that slot's patch - reproducing
/// the stale entries found in the corpus.
BuiltBank makeBank(const std::vector<BankEntry>& entries, std::uint32_t baseAddress = 0x00344E70,
                   int staleKitPointerSlot = -1);

} // namespace k5000::test
