#pragma once

// Read-only KA1 parser.
//
// Nothing in this header writes, repairs or normalizes. The checksum is
// computed for reporting only; it is never stored back. Writing stays
// Unsupported until a serializer exists with its own round-trip tests
// (specification section 25).

#include "k5000/core/VerificationLevel.h"
#include "k5000/ka1/Single.h"

#include <cstdint>
#include <span>

namespace k5000::ka1 {

/// Kawai Single Tone checksum:
///   (sum(common excluding byte 0) + sum(active source descriptors) + 0xA5) & 0x7F
///
/// The ADD wave kits are excluded; each carries its own checksum in its first
/// byte. Verified against 937 of 937 structurally valid corpus files.
/// `payload` must hold at least `patchSize(sources, 0)` bytes.
[[nodiscard]] std::uint8_t computeChecksum(std::span<const std::uint8_t> payload, int sources);

/// Checksum of one ADD wave kit: (sum(kit[1:]) + 0xA5) & 0x7F.
/// `kit` must be exactly kAddWaveKitSize bytes.
[[nodiscard]] std::uint8_t computeAddWaveKitChecksum(std::span<const std::uint8_t> kit);

struct ParseOptions {
    /// A standalone .KA1 file must end exactly where its structure ends;
    /// trailing bytes are reported. A patch embedded in a bank is followed by
    /// unrelated data, so the caller sets this.
    bool embedded = false;

    /// Verify each ADD wave kit's own checksum. Cheap and worth doing.
    bool verifyAddWaveKitChecksums = true;
};

struct ParseResult {
    Single single;

    /// True when the structure could be traversed far enough to know the exact
    /// patch length. A checksum mismatch does not clear this.
    bool structureKnown = false;

    [[nodiscard]] ValidationState state() const noexcept { return single.validationState; }
};

/// Parse a patch payload. Never reads outside `bytes`.
[[nodiscard]] ParseResult parse(std::span<const std::uint8_t> bytes, const ParseOptions& options = {});

/// Capability descriptor for this module (docs/OPEN_QUESTIONS.md Q4).
[[nodiscard]] const FormatSupport& support() noexcept;

} // namespace k5000::ka1
