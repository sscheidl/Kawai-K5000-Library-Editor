#pragma once

// KA1 / K5000 single patch layout.
//
// Every constant here is verified against the private reference corpus and
// recorded with its evidence in docs/FORMAT_NOTES.md. Reproduce with:
//   python tools/research/ka1_checksum_probe.py <corpus>
//   python tools/research/ka1_source_probe.py   <corpus>
//
//   patch = common(82) + source_descriptor(86) * S + add_wave_kit(806) * A
//
// S is stored at common offset 0x33.
// A is the number of sources whose wave kit number is 512.
//
// A is NOT taken from the KAA pointer table: that table carries stale entries
// left by previously resident patches. See docs/FORMAT_NOTES.md, "Invariant A".

#include <cstddef>
#include <cstdint>

namespace k5000::ka1 {

inline constexpr std::size_t kCommonSize = 82;
inline constexpr std::size_t kSourceDescriptorSize = 86;
inline constexpr std::size_t kAddWaveKitSize = 806;

inline constexpr std::size_t kChecksumOffset = 0x00;
inline constexpr std::size_t kNameOffset = 0x28;
inline constexpr std::size_t kNameLength = 8;
inline constexpr std::size_t kSourceCountOffset = 0x33;

/// Wave kit number inside a source descriptor: two 7-bit bytes, high first.
inline constexpr std::size_t kWaveKitOffsetInSource = 28;
inline constexpr std::uint16_t kAdditiveWaveKit = 512;

/// Highest wave kit number that can be encoded meaningfully. Anything above
/// kAdditiveWaveKit is out of range; the corpus contains exactly one such value
/// and that file is structurally ambiguous.
inline constexpr std::uint16_t kMaxWaveKit = kAdditiveWaveKit;

inline constexpr std::uint8_t kChecksumSeed = 0xA5;
inline constexpr int kMinSources = 1;
inline constexpr int kMaxSources = 6;

/// Smallest and largest structurally possible patch.
inline constexpr std::size_t kMinPatchSize = kCommonSize + kSourceDescriptorSize * kMinSources;
inline constexpr std::size_t kMaxPatchSize =
    kCommonSize + (kSourceDescriptorSize + kAddWaveKitSize) * kMaxSources;

constexpr std::size_t patchSize(int sources, int addSources) noexcept {
    return kCommonSize + kSourceDescriptorSize * static_cast<std::size_t>(sources)
           + kAddWaveKitSize * static_cast<std::size_t>(addSources);
}

constexpr std::size_t sourceDescriptorOffset(int index) noexcept {
    return kCommonSize + kSourceDescriptorSize * static_cast<std::size_t>(index);
}

constexpr std::size_t addWaveKitOffset(int sources, int kitIndex) noexcept {
    return kCommonSize + kSourceDescriptorSize * static_cast<std::size_t>(sources)
           + kAddWaveKitSize * static_cast<std::size_t>(kitIndex);
}

} // namespace k5000::ka1
