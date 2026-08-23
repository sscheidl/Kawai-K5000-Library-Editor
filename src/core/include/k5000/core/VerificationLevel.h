#pragma once

// Verification levels, decided in docs/OPEN_QUESTIONS.md Q4 (2026-08-23).
//
// This is deliberately a property of the code, not a claim in a document. The
// GUI and the export log derive their wording from it, and a test asserts that
// every GoldenTested claim has a corresponding round-trip and golden test.

#include <string_view>

namespace k5000 {

enum class VerificationLevel {
    Unsupported = 0,     // not implemented; no semantic claim, bytes preserved
    Experimental = 1,    // implemented, not reproduced against reference files
    Observed = 2,        // reproduced consistently across reference files
    GoldenTested = 3,    // round-trip and golden tests pass against the corpus
    HardwareVerified = 4 // confirmed on the real K5000S, logged in HARDWARE_ACCEPTANCE.md
};

// A conversion is capped by the weaker of its two endpoints.
constexpr VerificationLevel weaker(VerificationLevel a, VerificationLevel b) noexcept {
    return (a < b) ? a : b;
}

// Anything below GoldenTested is presented as experimental and, for writing,
// disabled unless the user explicitly enables it.
constexpr bool isSupportedForWriting(VerificationLevel level) noexcept {
    return level >= VerificationLevel::GoldenTested;
}

std::string_view toString(VerificationLevel level) noexcept;

/// Per-format capability descriptor. Parsing routinely leads writing, so the
/// axes are tracked independently.
struct FormatSupport {
    std::string_view format;
    VerificationLevel parsing = VerificationLevel::Unsupported;
    std::string_view parsingEvidence;
    VerificationLevel writing = VerificationLevel::Unsupported;
    std::string_view writingEvidence;
    VerificationLevel conversion = VerificationLevel::Unsupported;
    std::string_view conversionEvidence;
};

} // namespace k5000
