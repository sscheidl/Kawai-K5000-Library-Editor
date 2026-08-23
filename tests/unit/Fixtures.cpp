#include "Fixtures.h"

#include "k5000/kaa/Bank.h"

#include <algorithm>
#include <numeric>

namespace k5000::test {
namespace {

std::uint8_t patchChecksum(const std::vector<std::uint8_t>& payload, int sources) {
    const std::size_t end =
        ka1::kCommonSize + ka1::kSourceDescriptorSize * static_cast<std::size_t>(sources);
    std::uint32_t total = ka1::kChecksumSeed;
    for (std::size_t i = 1; i < end; ++i) {
        total += payload[i];
    }
    return static_cast<std::uint8_t>(total & 0x7F);
}

void writeBigEndian32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
    bytes[offset] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
    bytes[offset + 1] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
    bytes[offset + 2] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
    bytes[offset + 3] = static_cast<std::uint8_t>(value & 0xFF);
}

} // namespace

std::vector<std::uint8_t> makeSingle(const std::string& name,
                                     const std::vector<SourceSpec>& sources, std::uint8_t filler) {
    const int sourceCount = static_cast<int>(sources.size());
    const int addCount =
        static_cast<int>(std::count_if(sources.begin(), sources.end(),
                                       [](const SourceSpec& s) { return s.additive; }));

    std::vector<std::uint8_t> payload(ka1::patchSize(sourceCount, addCount), filler);

    // Common block.
    payload[ka1::kSourceCountOffset] = static_cast<std::uint8_t>(sourceCount);
    payload[0x27] = 0x00; // constant in every corpus file
    for (std::size_t i = 0; i < ka1::kNameLength; ++i) {
        payload[ka1::kNameOffset + i] =
            i < name.size() ? static_cast<std::uint8_t>(name[i]) : static_cast<std::uint8_t>(' ');
    }

    // Source descriptors: wave kit number, two 7-bit bytes, high first.
    for (int i = 0; i < sourceCount; ++i) {
        const std::size_t waveOffset =
            ka1::sourceDescriptorOffset(i) + ka1::kWaveKitOffsetInSource;
        const std::uint16_t wave =
            sources[static_cast<std::size_t>(i)].additive
                ? ka1::kAdditiveWaveKit
                : sources[static_cast<std::size_t>(i)].pcmWaveKit;
        payload[waveOffset] = static_cast<std::uint8_t>((wave >> 7) & 0x7F);
        payload[waveOffset + 1] = static_cast<std::uint8_t>(wave & 0x7F);
    }

    // ADD wave kits, each with its own checksum in its first byte. Vary the
    // content so two kits are not accidentally identical.
    for (int kit = 0; kit < addCount; ++kit) {
        const std::size_t start = ka1::addWaveKitOffset(sourceCount, kit);
        for (std::size_t i = 1; i < ka1::kAddWaveKitSize; ++i) {
            payload[start + i] = static_cast<std::uint8_t>((i + static_cast<std::size_t>(kit) * 7) & 0x7F);
        }
        std::uint32_t total = ka1::kChecksumSeed;
        for (std::size_t i = 1; i < ka1::kAddWaveKitSize; ++i) {
            total += payload[start + i];
        }
        payload[start] = static_cast<std::uint8_t>(total & 0x7F);
    }

    payload[ka1::kChecksumOffset] = patchChecksum(payload, sourceCount);
    return payload;
}

void refreshChecksum(std::vector<std::uint8_t>& payload) {
    const int sources = static_cast<int>(payload[ka1::kSourceCountOffset]);
    payload[ka1::kChecksumOffset] = patchChecksum(payload, sources);
}

BuiltBank makeBank(const std::vector<BankEntry>& entries, std::uint32_t baseAddress,
                   int staleKitPointerSlot) {
    BuiltBank built;
    built.bytes.assign(kaa::kFileSize, 0x00);
    built.baseAddress = baseAddress;

    std::size_t cursor = kaa::kHeaderSize;
    std::uint32_t highWater = baseAddress;

    for (const BankEntry& entry : entries) {
        // Dead space before the patch. A parser that sizes a patch by the
        // distance to the next pointer will swallow this.
        for (std::size_t i = 0; i < entry.gapBefore; ++i) {
            built.bytes[cursor + i] = 0x7F;
        }
        cursor += entry.gapBefore;

        const std::uint32_t pointer =
            baseAddress + static_cast<std::uint32_t>(cursor - kaa::kHeaderSize);
        const std::size_t rowOffset =
            static_cast<std::size_t>(entry.slot) * kaa::kPointersPerSlot * 4;
        writeBigEndian32(built.bytes, rowOffset, pointer);

        const int sourceCount = static_cast<int>(entry.payload[ka1::kSourceCountOffset]);
        int kitIndex = 0;
        for (int i = 0; i < sourceCount; ++i) {
            const std::size_t waveOffset =
                ka1::sourceDescriptorOffset(i) + ka1::kWaveKitOffsetInSource;
            const std::uint16_t wave = static_cast<std::uint16_t>(
                (entry.payload[waveOffset] << 7) | entry.payload[waveOffset + 1]);
            if (wave == ka1::kAdditiveWaveKit) {
                const std::uint32_t kitPointer =
                    pointer + static_cast<std::uint32_t>(ka1::addWaveKitOffset(sourceCount, kitIndex));
                writeBigEndian32(built.bytes, rowOffset + static_cast<std::size_t>(kitIndex + 1) * 4,
                                 kitPointer);
                ++kitIndex;
            }
        }

        if (entry.slot == staleKitPointerSlot && sourceCount < 6) {
            // A pointer for a source that does not exist, as left behind by a
            // larger patch that previously occupied this slot.
            writeBigEndian32(built.bytes, rowOffset + static_cast<std::size_t>(sourceCount + 1) * 4,
                             pointer + 0x100);
        }

        std::copy(entry.payload.begin(), entry.payload.end(), built.bytes.begin() + static_cast<std::ptrdiff_t>(cursor));
        built.offsets.push_back(cursor);
        cursor += entry.payload.size();
        highWater = baseAddress + static_cast<std::uint32_t>(cursor - kaa::kHeaderSize);
    }

    writeBigEndian32(built.bytes, kaa::kEndOfDataPointerOffset, highWater);
    return built;
}

} // namespace k5000::test
