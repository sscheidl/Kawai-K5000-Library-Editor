#pragma once

// KAA single-bank reader and byte-exact extraction.
//
// Container layout, verified against 76 of 76 reference banks:
//
//   0x0000  pointer table, 128 slots x 7 big-endian dwords   3584 bytes
//   0x0E00  end-of-used-data pointer                            4 bytes
//   0x0E04  patch data region                              131072 bytes
//   ------                                                 -------------
//                                                          134660 bytes
//
//   base        = smallest non-zero pointer in the table
//   file_offset = 0x0E04 + (pointer - base)
//
// Validated by parsing the patch at every computed offset and checking its
// Kawai checksum: 4072 of 4072 pass. A wrong base would destroy every one.
//
// INVARIANT A. Pointers locate patch STARTS. They must never determine patch
// LENGTH, in either of these two forms:
//
//     size = next_pointer - pointer            // forbidden: banks are fragmented
//     size = f(count of ADD-kit pointers)      // forbidden: entries go stale
//
// 8 of the corpus banks are fragmented, and 28 slots carry an ADD-kit pointer
// beyond their own source count - left behind by a larger patch that used to
// occupy the slot. Length comes from the patch structure, nowhere else.
//
// Reproduce with: python tools/research/kaa_pointer_probe.py <corpus>

#include "k5000/core/Diagnostics.h"
#include "k5000/core/VerificationLevel.h"
#include "k5000/ka1/Single.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace k5000::kaa {

inline constexpr int kSlotCount = 128;
inline constexpr int kPointersPerSlot = 7;
inline constexpr std::size_t kHeaderSize = kSlotCount * kPointersPerSlot * 4 + 4; // 0x0E04
inline constexpr std::size_t kDataRegionSize = 0x20000;                           // 131072
inline constexpr std::size_t kFileSize = kHeaderSize + kDataRegionSize;           // 134660
inline constexpr std::size_t kEndOfDataPointerOffset = kSlotCount * kPointersPerSlot * 4;

/// Bank capacity is bounded twice: by slots and by the byte budget. The byte
/// budget binds first in practice. Per decision Q3 this produces a warning, not
/// a hard export gate, until the model is GoldenTested.
inline constexpr int kMaxPatches = kSlotCount;
inline constexpr std::size_t kPatchDataBudget = kDataRegionSize;

struct Slot {
    int index = 0;
    std::uint32_t patchPointer = 0;
    std::array<std::uint32_t, 6> addWaveKitPointers{};

    [[nodiscard]] bool occupied() const noexcept { return patchPointer != 0; }

    /// How many ADD-kit pointers are set. Diagnostic only - never used for
    /// sizing. See Invariant A above.
    [[nodiscard]] int addWaveKitPointerCount() const noexcept;
};

struct Bank {
    std::array<Slot, kSlotCount> slots{};
    std::uint32_t endOfDataPointer = 0;
    std::uint32_t baseAddress = 0;

    ValidationState validationState = ValidationState::Malformed;
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] int occupiedSlotCount() const noexcept;

    /// Bytes of the data region reported as used by the bank itself.
    [[nodiscard]] std::size_t usedBytes() const noexcept;
    [[nodiscard]] std::size_t freeBytes() const noexcept;

    /// File offset of a pointer, or nullopt when it falls outside the data
    /// region.
    [[nodiscard]] std::optional<std::size_t> fileOffset(std::uint32_t pointer) const noexcept;
};

struct BankResult {
    Bank bank;
    bool usable = false; ///< the pointer table could be read and a base derived
};

[[nodiscard]] BankResult parseBank(std::span<const std::uint8_t> bytes);

/// One patch lifted out of a bank.
struct ExtractedPatch {
    int slot = 0;
    std::size_t fileOffset = 0;
    ka1::Single single;

    /// Exactly `single.structuralSize` bytes, copied verbatim from the bank.
    /// Extraction is byte preservation: nothing is normalized, recomputed or
    /// reordered (specification section 5).
    [[nodiscard]] const std::vector<std::uint8_t>& payload() const noexcept {
        return single.rawPayload;
    }
};

struct ExtractionResult {
    std::vector<ExtractedPatch> patches;
    std::vector<Diagnostic> diagnostics;
    ValidationState validationState = ValidationState::Valid;

    /// Slots that are occupied but could not be read. Never silently dropped.
    std::vector<int> failedSlots;
};

/// Extract one occupied slot.
[[nodiscard]] std::optional<ExtractedPatch> extractSlot(const Bank& bank,
                                                        std::span<const std::uint8_t> bytes,
                                                        int slotIndex, DiagnosticSink& sink);

/// Extract every occupied slot. Failures are reported, never discarded.
[[nodiscard]] ExtractionResult extractAll(const Bank& bank, std::span<const std::uint8_t> bytes);

[[nodiscard]] const FormatSupport& support() noexcept;

} // namespace k5000::kaa
