#include "k5000/core/Diagnostics.h"
#include "k5000/core/VerificationLevel.h"

#include <algorithm>

namespace k5000 {

std::string_view toString(ValidationState state) noexcept {
    switch (state) {
    case ValidationState::Valid: return "Valid";
    case ValidationState::ChecksumMismatch: return "ChecksumMismatch";
    case ValidationState::StructurallyAmbiguous: return "StructurallyAmbiguous";
    case ValidationState::UnsupportedVariant: return "UnsupportedVariant";
    case ValidationState::Truncated: return "Truncated";
    case ValidationState::Malformed: return "Malformed";
    }
    return "Malformed";
}

std::string_view toString(VerificationLevel level) noexcept {
    switch (level) {
    case VerificationLevel::Unsupported: return "Unsupported";
    case VerificationLevel::Experimental: return "Experimental";
    case VerificationLevel::Observed: return "Observed";
    case VerificationLevel::GoldenTested: return "GoldenTested";
    case VerificationLevel::HardwareVerified: return "HardwareVerified";
    }
    return "Unsupported";
}

std::string_view toString(DiagnosticCode code) noexcept {
    switch (code) {
    case DiagnosticCode::TruncatedCommonBlock: return "truncated common block";
    case DiagnosticCode::TruncatedSourceDescriptors: return "truncated source descriptors";
    case DiagnosticCode::TruncatedAddWaveKit: return "truncated ADD wave kit";
    case DiagnosticCode::SourceCountOutOfRange: return "source count out of range";
    case DiagnosticCode::TrailingBytes: return "trailing bytes after the patch";
    case DiagnosticCode::PatchChecksumMismatch: return "patch checksum mismatch";
    case DiagnosticCode::AddWaveKitChecksumMismatch: return "ADD wave kit checksum mismatch";
    case DiagnosticCode::LengthConflictsWithSourceDescriptors:
        return "file length and source descriptors disagree on the ADD source count";
    case DiagnosticCode::WaveKitNumberOutOfRange: return "wave kit number out of range";
    case DiagnosticCode::BankUnexpectedFileSize: return "unexpected bank file size";
    case DiagnosticCode::BankNoOccupiedSlots: return "bank has no occupied slot";
    case DiagnosticCode::BankPointerBelowBase: return "pointer below the bank base address";
    case DiagnosticCode::BankPointerOutsideDataRegion: return "pointer outside the data region";
    case DiagnosticCode::BankPatchExceedsDataRegion: return "patch extends past the data region";
    case DiagnosticCode::BankStaleAddKitPointer:
        return "ADD wave kit pointer set beyond the source count (stale)";
    case DiagnosticCode::BankSlotUnreadable: return "slot could not be read";
    }
    return "unknown diagnostic";
}

std::string Diagnostic::message() const {
    std::string text;
    if (!format.empty()) {
        text += std::string(format);
        text += ": ";
    }
    text += std::string(toString(code));
    if (offset) {
        text += " at offset 0x";
        constexpr char digits[] = "0123456789ABCDEF";
        std::string hex;
        auto value = *offset;
        do {
            hex.insert(hex.begin(), digits[value & 0xF]);
            value >>= 4;
        } while (value != 0);
        text += hex;
    }
    if (expected || actual) {
        text += " (expected ";
        text += expected ? std::to_string(*expected) : std::string("-");
        text += ", actual ";
        text += actual ? std::to_string(*actual) : std::string("-");
        text += ")";
    }
    if (!detail.empty()) {
        text += " - ";
        text += detail;
    }
    return text;
}

void DiagnosticSink::add(Diagnostic diagnostic) {
    state_ = worst(state_, diagnostic.severity);
    entries_.push_back(std::move(diagnostic));
}

bool DiagnosticSink::has(DiagnosticCode code) const noexcept {
    return std::any_of(entries_.begin(), entries_.end(),
                       [code](const Diagnostic& d) { return d.code == code; });
}

} // namespace k5000
