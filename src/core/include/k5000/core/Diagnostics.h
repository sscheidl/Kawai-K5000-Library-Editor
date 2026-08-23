#pragma once

// Structured diagnostics. Specification section 35: malformed input is reported
// with source, detected format, offset, expected condition and actual
// condition - never as raw exception text, and never auto-repaired.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace k5000 {

/// How much a reader is willing to say about a payload it has looked at.
/// Reaching the end of the input is NOT by itself a reason to say Valid.
/// Ordered by how unusable the payload is. Higher wins when states combine, so
/// the order encodes a judgement: an ambiguous extent is more dangerous than an
/// unrecognised variant, because it means two readings disagree about which
/// bytes belong to the patch at all.
enum class ValidationState {
    Valid = 0,
    ChecksumMismatch = 1,      // structure understood, stored checksum disagrees
    UnsupportedVariant = 2,    // recognisable but outside what has been verified
    StructurallyAmbiguous = 3, // two independent structural readings disagree
    Truncated = 4,             // ends before the structure it declares
    Malformed = 5              // cannot be traversed at all
};

/// Worse state wins when several observations are combined.
constexpr ValidationState worst(ValidationState a, ValidationState b) noexcept {
    return (a > b) ? a : b;
}

std::string_view toString(ValidationState state) noexcept;

enum class DiagnosticCode {
    TruncatedCommonBlock,
    TruncatedSourceDescriptors,
    TruncatedAddWaveKit,
    SourceCountOutOfRange,
    TrailingBytes,
    PatchChecksumMismatch,
    AddWaveKitChecksumMismatch,
    LengthConflictsWithSourceDescriptors,
    WaveKitNumberOutOfRange,
    BankUnexpectedFileSize,
    BankNoOccupiedSlots,
    BankPointerBelowBase,
    BankPointerOutsideDataRegion,
    BankPatchExceedsDataRegion,
    BankStaleAddKitPointer,
    BankSlotUnreadable
};

std::string_view toString(DiagnosticCode code) noexcept;

/// One reported problem. `expected` and `actual` are free-form numeric context
/// whose meaning depends on the code; `detail` carries anything that does not
/// fit. The originating file name is added by the caller, which is the layer
/// that knows it.
struct Diagnostic {
    DiagnosticCode code{};
    ValidationState severity = ValidationState::Malformed;
    std::string_view format;              // "KA1", "KAA", ...
    std::optional<std::size_t> offset;    // byte offset within the input
    std::optional<std::int64_t> expected;
    std::optional<std::int64_t> actual;
    std::string detail;

    /// Human-readable one-liner, without the file name.
    [[nodiscard]] std::string message() const;
};

/// Collects diagnostics and tracks the worst severity seen.
class DiagnosticSink {
public:
    void add(Diagnostic diagnostic);

    [[nodiscard]] const std::vector<Diagnostic>& entries() const noexcept { return entries_; }
    [[nodiscard]] ValidationState state() const noexcept { return state_; }
    [[nodiscard]] bool empty() const noexcept { return entries_.empty(); }
    [[nodiscard]] bool has(DiagnosticCode code) const noexcept;

    void observe(ValidationState state) noexcept { state_ = worst(state_, state); }

private:
    std::vector<Diagnostic> entries_;
    ValidationState state_ = ValidationState::Valid;
};

} // namespace k5000
