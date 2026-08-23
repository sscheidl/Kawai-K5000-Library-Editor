#include "k5000/kaa/Bank.h"

#include "k5000/ka1/Parser.h"

#include <algorithm>
#include <limits>

namespace k5000::kaa {
namespace {

constexpr std::string_view kFormat = "KAA";

Diagnostic makeDiagnostic(DiagnosticCode code, ValidationState severity,
                          std::optional<std::size_t> offset = std::nullopt,
                          std::optional<std::int64_t> expected = std::nullopt,
                          std::optional<std::int64_t> actual = std::nullopt,
                          std::string detail = {}) {
    Diagnostic d;
    d.code = code;
    d.severity = severity;
    d.format = kFormat;
    d.offset = offset;
    d.expected = expected;
    d.actual = actual;
    d.detail = std::move(detail);
    return d;
}

std::uint32_t readBigEndian32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return (static_cast<std::uint32_t>(bytes[offset]) << 24)
           | (static_cast<std::uint32_t>(bytes[offset + 1]) << 16)
           | (static_cast<std::uint32_t>(bytes[offset + 2]) << 8)
           | static_cast<std::uint32_t>(bytes[offset + 3]);
}

} // namespace

int Slot::addWaveKitPointerCount() const noexcept {
    return static_cast<int>(
        std::count_if(addWaveKitPointers.begin(), addWaveKitPointers.end(),
                      [](std::uint32_t p) { return p != 0; }));
}

int Bank::occupiedSlotCount() const noexcept {
    return static_cast<int>(
        std::count_if(slots.begin(), slots.end(), [](const Slot& s) { return s.occupied(); }));
}

std::size_t Bank::usedBytes() const noexcept {
    return endOfDataPointer >= baseAddress ? endOfDataPointer - baseAddress : 0;
}

std::size_t Bank::freeBytes() const noexcept {
    const std::size_t used = usedBytes();
    return used >= kPatchDataBudget ? 0 : kPatchDataBudget - used;
}

std::optional<std::size_t> Bank::fileOffset(std::uint32_t pointer) const noexcept {
    if (pointer < baseAddress) {
        return std::nullopt;
    }
    const std::size_t relative = pointer - baseAddress;
    if (relative >= kDataRegionSize) {
        return std::nullopt;
    }
    return kHeaderSize + relative;
}

const FormatSupport& support() noexcept {
    static const FormatSupport descriptor{
        "KAA",
        VerificationLevel::GoldenTested,
        "Container layout and pointer mapping verified across 76 of 76 corpus "
        "banks, validated by the patch checksum at every one of 4072 computed "
        "offsets, and asserted by the corpus-gated golden tests. Extraction "
        "reproduces 2210 standalone KA1 files byte for byte.",
        VerificationLevel::Unsupported,
        "No bank writer exists. Rebuilding requires proven patch boundaries and "
        "a verified allocation rule.",
        VerificationLevel::Unsupported,
        "KAA to KA1 extraction is byte-exact copy, not a conversion, and is "
        "reported under parsing."};
    return descriptor;
}

BankResult parseBank(std::span<const std::uint8_t> bytes) {
    BankResult result;
    Bank& bank = result.bank;
    DiagnosticSink sink;

    if (bytes.size() != kFileSize) {
        sink.add(makeDiagnostic(DiagnosticCode::BankUnexpectedFileSize, ValidationState::Malformed,
                                0, static_cast<std::int64_t>(kFileSize),
                                static_cast<std::int64_t>(bytes.size())));
        bank.validationState = sink.state();
        bank.diagnostics = sink.entries();
        return result;
    }

    std::uint32_t base = std::numeric_limits<std::uint32_t>::max();
    bool anyPointer = false;

    for (int slotIndex = 0; slotIndex < kSlotCount; ++slotIndex) {
        Slot& slot = bank.slots[static_cast<std::size_t>(slotIndex)];
        slot.index = slotIndex;
        const std::size_t rowOffset = static_cast<std::size_t>(slotIndex) * kPointersPerSlot * 4;

        slot.patchPointer = readBigEndian32(bytes, rowOffset);
        for (int p = 0; p < 6; ++p) {
            slot.addWaveKitPointers[static_cast<std::size_t>(p)] =
                readBigEndian32(bytes, rowOffset + static_cast<std::size_t>(p + 1) * 4);
        }

        // The base is the smallest non-zero pointer anywhere in the table,
        // including ADD-kit pointers - a stale kit pointer can still be the
        // lowest address the bank ever used.
        if (slot.patchPointer != 0) {
            base = std::min(base, slot.patchPointer);
            anyPointer = true;
        }
        for (std::uint32_t pointer : slot.addWaveKitPointers) {
            if (pointer != 0) {
                base = std::min(base, pointer);
                anyPointer = true;
            }
        }
    }

    bank.endOfDataPointer = readBigEndian32(bytes, kEndOfDataPointerOffset);

    if (!anyPointer) {
        sink.add(makeDiagnostic(DiagnosticCode::BankNoOccupiedSlots, ValidationState::Malformed));
        bank.validationState = sink.state();
        bank.diagnostics = sink.entries();
        return result;
    }

    bank.baseAddress = base;
    result.usable = true;

    // Report stale ADD-kit pointers. They are never used for sizing, but they
    // are worth surfacing: they mean the slot previously held a larger patch.
    for (const Slot& slot : bank.slots) {
        if (!slot.occupied()) {
            continue;
        }
        const auto offset = bank.fileOffset(slot.patchPointer);
        if (!offset) {
            continue;
        }
        const std::size_t sourceCountOffset = *offset + ka1::kSourceCountOffset;
        if (sourceCountOffset >= bytes.size()) {
            continue;
        }
        const int sources = static_cast<int>(bytes[sourceCountOffset]);
        if (sources < ka1::kMinSources || sources > ka1::kMaxSources) {
            continue;
        }
        for (int p = sources; p < 6; ++p) {
            if (slot.addWaveKitPointers[static_cast<std::size_t>(p)] != 0) {
                sink.add(makeDiagnostic(DiagnosticCode::BankStaleAddKitPointer,
                                        ValidationState::UnsupportedVariant,
                                        static_cast<std::size_t>(slot.index) * kPointersPerSlot * 4,
                                        sources, p + 1,
                                        "slot " + std::to_string(slot.index + 1)
                                            + ": ADD kit pointer for a source that does not exist"));
                break;
            }
        }
    }

    bank.validationState = sink.state();
    bank.diagnostics = sink.entries();
    return result;
}

std::optional<ExtractedPatch> extractSlot(const Bank& bank, std::span<const std::uint8_t> bytes,
                                          int slotIndex, DiagnosticSink& sink) {
    if (slotIndex < 0 || slotIndex >= kSlotCount || bytes.size() != kFileSize) {
        sink.add(makeDiagnostic(DiagnosticCode::BankSlotUnreadable, ValidationState::Malformed));
        return std::nullopt;
    }

    const Slot& slot = bank.slots[static_cast<std::size_t>(slotIndex)];
    if (!slot.occupied()) {
        return std::nullopt;
    }

    if (slot.patchPointer < bank.baseAddress) {
        sink.add(makeDiagnostic(DiagnosticCode::BankPointerBelowBase, ValidationState::Malformed,
                                std::nullopt, bank.baseAddress, slot.patchPointer,
                                "slot " + std::to_string(slotIndex + 1)));
        return std::nullopt;
    }

    const auto offset = bank.fileOffset(slot.patchPointer);
    if (!offset) {
        sink.add(makeDiagnostic(DiagnosticCode::BankPointerOutsideDataRegion,
                                ValidationState::Malformed, std::nullopt,
                                static_cast<std::int64_t>(kDataRegionSize),
                                slot.patchPointer - bank.baseAddress,
                                "slot " + std::to_string(slotIndex + 1)));
        return std::nullopt;
    }

    // The patch may use no more than the rest of the data region. Parsing is
    // bounded by that span, so a corrupt source count cannot read past it.
    const std::size_t available = bytes.size() - *offset;
    ka1::ParseOptions options;
    options.embedded = true; // the bank continues after this patch
    const ka1::ParseResult parsed = ka1::parse(bytes.subspan(*offset, available), options);

    for (const Diagnostic& diagnostic : parsed.single.diagnostics) {
        Diagnostic relocated = diagnostic;
        if (relocated.offset) {
            relocated.offset = *relocated.offset + *offset;
        }
        relocated.detail = relocated.detail.empty()
                               ? "slot " + std::to_string(slotIndex + 1)
                               : "slot " + std::to_string(slotIndex + 1) + ": " + relocated.detail;
        sink.add(relocated);
    }

    if (!parsed.structureKnown) {
        sink.add(makeDiagnostic(DiagnosticCode::BankSlotUnreadable, ValidationState::Malformed,
                                *offset, std::nullopt, std::nullopt,
                                "slot " + std::to_string(slotIndex + 1)
                                    + ": patch structure could not be determined"));
        return std::nullopt;
    }

    if (*offset + parsed.single.structuralSize > kHeaderSize + kDataRegionSize) {
        sink.add(makeDiagnostic(DiagnosticCode::BankPatchExceedsDataRegion,
                                ValidationState::Malformed, *offset,
                                static_cast<std::int64_t>(kHeaderSize + kDataRegionSize),
                                static_cast<std::int64_t>(*offset + parsed.single.structuralSize),
                                "slot " + std::to_string(slotIndex + 1)));
        return std::nullopt;
    }

    ExtractedPatch patch;
    patch.slot = slotIndex;
    patch.fileOffset = *offset;
    patch.single = parsed.single;
    return patch;
}

ExtractionResult extractAll(const Bank& bank, std::span<const std::uint8_t> bytes) {
    ExtractionResult result;
    DiagnosticSink sink;

    for (int slotIndex = 0; slotIndex < kSlotCount; ++slotIndex) {
        if (!bank.slots[static_cast<std::size_t>(slotIndex)].occupied()) {
            continue;
        }
        auto patch = extractSlot(bank, bytes, slotIndex, sink);
        if (patch) {
            result.patches.push_back(std::move(*patch));
        } else {
            result.failedSlots.push_back(slotIndex);
        }
    }

    result.diagnostics = sink.entries();
    result.validationState = sink.state();
    return result;
}

} // namespace k5000::kaa
