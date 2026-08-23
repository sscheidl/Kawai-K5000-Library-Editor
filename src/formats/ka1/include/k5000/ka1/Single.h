#pragma once

// Canonical in-memory single patch (specification section 4).
//
// Retains the raw payload alongside the parsed view, so an unmodified patch can
// always be written back byte for byte. Unknown and reserved bytes are carried,
// never normalized.

#include "k5000/core/Diagnostics.h"
#include "k5000/ka1/Layout.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace k5000::ka1 {

enum class SourceType { Additive, Pcm };

std::string_view toString(SourceType type) noexcept;

struct SourceDescriptor {
    int index = 0;
    std::size_t offset = 0;    // relative to the start of the patch
    std::uint16_t waveKit = 0; // 512 == additive
    SourceType type = SourceType::Pcm;
};

/// One K5000 single patch.
struct Single {
    // --- parsed metadata -------------------------------------------------
    std::array<std::uint8_t, kNameLength> rawName{};
    std::string name; // display form; bytes outside 0x20..0x7E shown as '?'
    int sourceCount = 0;
    std::vector<SourceDescriptor> sources;
    int addSourceCount = 0;

    /// Length derived exclusively from the patch's own structure. This is the
    /// only figure that may be used to bound an extraction.
    std::size_t structuralSize = 0;

    // --- preservation ----------------------------------------------------
    std::vector<std::uint8_t> rawPayload;

    // --- diagnostics -----------------------------------------------------
    std::uint8_t checksumStored = 0;
    std::uint8_t checksumCalculated = 0;
    ValidationState validationState = ValidationState::Malformed;
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] bool checksumMatches() const noexcept {
        return checksumStored == checksumCalculated;
    }

    /// Name with trailing padding removed, for lists and file names.
    [[nodiscard]] std::string trimmedName() const;
};

} // namespace k5000::ka1
