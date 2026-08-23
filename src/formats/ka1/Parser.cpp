#include "k5000/ka1/Parser.h"

#include <numeric>

namespace k5000::ka1 {
namespace {

constexpr std::string_view kFormat = "KA1";

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

std::uint32_t sumBytes(std::span<const std::uint8_t> bytes) {
    return std::accumulate(bytes.begin(), bytes.end(), std::uint32_t{0},
                           [](std::uint32_t acc, std::uint8_t b) { return acc + b; });
}

std::string makeDisplayName(std::span<const std::uint8_t> raw) {
    std::string name;
    name.reserve(raw.size());
    for (std::uint8_t b : raw) {
        name.push_back((b >= 0x20 && b <= 0x7E) ? static_cast<char>(b) : '?');
    }
    return name;
}

} // namespace

std::string_view toString(SourceType type) noexcept {
    return type == SourceType::Additive ? "ADD" : "PCM";
}

std::string Single::trimmedName() const {
    std::string trimmed = name;
    while (!trimmed.empty() && trimmed.back() == ' ') {
        trimmed.pop_back();
    }
    return trimmed;
}

std::uint8_t computeChecksum(std::span<const std::uint8_t> payload, int sources) {
    const std::size_t end = kCommonSize + kSourceDescriptorSize * static_cast<std::size_t>(sources);
    if (payload.size() < end || end == 0) {
        return 0;
    }
    const std::uint32_t total = sumBytes(payload.subspan(1, end - 1)) + kChecksumSeed;
    return static_cast<std::uint8_t>(total & 0x7F);
}

std::uint8_t computeAddWaveKitChecksum(std::span<const std::uint8_t> kit) {
    if (kit.size() != kAddWaveKitSize) {
        return 0;
    }
    const std::uint32_t total = sumBytes(kit.subspan(1)) + kChecksumSeed;
    return static_cast<std::uint8_t>(total & 0x7F);
}

const FormatSupport& support() noexcept {
    static const FormatSupport descriptor{
        "KA1",
        VerificationLevel::GoldenTested,
        "Structure, checksum scope and ADD/PCM classification verified across "
        "the private corpus (937/937 patch checksums) and asserted by the "
        "corpus-gated golden tests in tests/golden. Those tests skip where no "
        "corpus is configured, so public CI does not re-establish this level.",
        VerificationLevel::Unsupported,
        "No serializer exists. Byte-exact copy only.",
        VerificationLevel::Unsupported,
        "No conversion implemented."};
    return descriptor;
}

ParseResult parse(std::span<const std::uint8_t> bytes, const ParseOptions& options) {
    ParseResult result;
    Single& single = result.single;
    DiagnosticSink sink;

    // --- common block ----------------------------------------------------
    if (bytes.size() < kCommonSize) {
        sink.add(makeDiagnostic(DiagnosticCode::TruncatedCommonBlock, ValidationState::Truncated, 0,
                                static_cast<std::int64_t>(kCommonSize),
                                static_cast<std::int64_t>(bytes.size())));
        single.validationState = sink.state();
        single.diagnostics = sink.entries();
        return result;
    }

    single.checksumStored = bytes[kChecksumOffset];
    for (std::size_t i = 0; i < kNameLength; ++i) {
        single.rawName[i] = bytes[kNameOffset + i];
    }
    single.name = makeDisplayName(single.rawName);

    const int sourceCount = static_cast<int>(bytes[kSourceCountOffset]);
    if (sourceCount < kMinSources || sourceCount > kMaxSources) {
        sink.add(makeDiagnostic(DiagnosticCode::SourceCountOutOfRange, ValidationState::Malformed,
                                kSourceCountOffset, kMaxSources, sourceCount,
                                "source count must be 1..6"));
        single.validationState = sink.state();
        single.diagnostics = sink.entries();
        return result;
    }
    single.sourceCount = sourceCount;

    // --- source descriptors ----------------------------------------------
    const std::size_t afterSources = patchSize(sourceCount, 0);
    if (bytes.size() < afterSources) {
        sink.add(makeDiagnostic(DiagnosticCode::TruncatedSourceDescriptors,
                                ValidationState::Truncated, kCommonSize,
                                static_cast<std::int64_t>(afterSources),
                                static_cast<std::int64_t>(bytes.size())));
        single.validationState = sink.state();
        single.diagnostics = sink.entries();
        return result;
    }

    single.sources.reserve(static_cast<std::size_t>(sourceCount));
    int addSources = 0;
    for (int i = 0; i < sourceCount; ++i) {
        SourceDescriptor descriptor;
        descriptor.index = i;
        descriptor.offset = sourceDescriptorOffset(i);
        const std::size_t waveOffset = descriptor.offset + kWaveKitOffsetInSource;
        descriptor.waveKit = static_cast<std::uint16_t>((bytes[waveOffset] << 7) | bytes[waveOffset + 1]);
        descriptor.type =
            (descriptor.waveKit == kAdditiveWaveKit) ? SourceType::Additive : SourceType::Pcm;
        if (descriptor.type == SourceType::Additive) {
            ++addSources;
        } else if (descriptor.waveKit > kMaxWaveKit) {
            // Reported, not repaired. The source stays classified as PCM and
            // the conflict surfaces below if the length disagrees.
            sink.add(makeDiagnostic(DiagnosticCode::WaveKitNumberOutOfRange,
                                    ValidationState::UnsupportedVariant, waveOffset, kMaxWaveKit,
                                    descriptor.waveKit,
                                    "source " + std::to_string(i + 1)));
        }
        single.sources.push_back(descriptor);
    }
    single.addSourceCount = addSources;

    // --- ADD wave kits ----------------------------------------------------
    const std::size_t structuralSize = patchSize(sourceCount, addSources);
    if (bytes.size() < structuralSize) {
        sink.add(makeDiagnostic(DiagnosticCode::TruncatedAddWaveKit, ValidationState::Truncated,
                                afterSources, static_cast<std::int64_t>(structuralSize),
                                static_cast<std::int64_t>(bytes.size())));
        single.validationState = sink.state();
        single.diagnostics = sink.entries();
        return result;
    }

    single.structuralSize = structuralSize;
    result.structureKnown = true;

    // --- length agreement -------------------------------------------------
    // For a standalone file the declared structure must account for every byte.
    // A mismatch that is a whole number of wave kits means the file length and
    // the descriptors disagree about how many sources are additive: two
    // independent readings, no way to choose between them without guessing.
    if (!options.embedded && bytes.size() != structuralSize) {
        const std::size_t declared = bytes.size();
        const bool wholeKits =
            declared > afterSources && (declared - afterSources) % kAddWaveKitSize == 0
            && (declared - afterSources) / kAddWaveKitSize <= static_cast<std::size_t>(sourceCount);
        if (wholeKits) {
            sink.add(makeDiagnostic(
                DiagnosticCode::LengthConflictsWithSourceDescriptors,
                ValidationState::StructurallyAmbiguous, 0,
                static_cast<std::int64_t>((declared - afterSources) / kAddWaveKitSize), addSources,
                "ADD source count from file length vs. from wave kit numbers"));
        } else {
            sink.add(makeDiagnostic(DiagnosticCode::TrailingBytes, ValidationState::Malformed,
                                    structuralSize, static_cast<std::int64_t>(structuralSize),
                                    static_cast<std::int64_t>(declared)));
        }
    }

    // --- checksums ---------------------------------------------------------
    single.checksumCalculated = computeChecksum(bytes, sourceCount);
    if (!single.checksumMatches()) {
        sink.add(makeDiagnostic(DiagnosticCode::PatchChecksumMismatch,
                                ValidationState::ChecksumMismatch, kChecksumOffset,
                                single.checksumCalculated, single.checksumStored));
    }

    if (options.verifyAddWaveKitChecksums) {
        for (int kit = 0; kit < addSources; ++kit) {
            const std::size_t offset = addWaveKitOffset(sourceCount, kit);
            const auto kitBytes = bytes.subspan(offset, kAddWaveKitSize);
            const std::uint8_t expected = computeAddWaveKitChecksum(kitBytes);
            if (kitBytes[0] != expected) {
                sink.add(makeDiagnostic(DiagnosticCode::AddWaveKitChecksumMismatch,
                                        ValidationState::ChecksumMismatch, offset, expected,
                                        kitBytes[0], "ADD wave kit " + std::to_string(kit + 1)));
            }
        }
    }

    // --- preservation ------------------------------------------------------
    // Exactly the structural extent, never more. Padding or neighbouring bank
    // data must not enter the payload.
    const auto payload = bytes.first(structuralSize);
    single.rawPayload.assign(payload.begin(), payload.end());

    single.validationState = sink.state();
    single.diagnostics = sink.entries();
    return result;
}

} // namespace k5000::ka1
