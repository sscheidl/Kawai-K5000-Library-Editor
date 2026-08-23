// The verification model is meant to be structural, not documentary
// (docs/OPEN_QUESTIONS.md Q4). These tests make the claims falsifiable.

#include "k5000/core/Diagnostics.h"
#include "k5000/core/VerificationLevel.h"
#include "k5000/ka1/Parser.h"
#include "k5000/kaa/Bank.h"

#include <catch2/catch_test_macros.hpp>

using namespace k5000;

TEST_CASE("verification levels are ordered and a conversion is capped by its weaker end",
          "[verification]") {
    CHECK(VerificationLevel::Unsupported < VerificationLevel::Experimental);
    CHECK(VerificationLevel::Experimental < VerificationLevel::Observed);
    CHECK(VerificationLevel::Observed < VerificationLevel::GoldenTested);
    CHECK(VerificationLevel::GoldenTested < VerificationLevel::HardwareVerified);

    CHECK(weaker(VerificationLevel::GoldenTested, VerificationLevel::Experimental)
          == VerificationLevel::Experimental);
    CHECK(weaker(VerificationLevel::Observed, VerificationLevel::Observed)
          == VerificationLevel::Observed);
}

TEST_CASE("writing is disabled below GoldenTested", "[verification]") {
    CHECK_FALSE(isSupportedForWriting(VerificationLevel::Unsupported));
    CHECK_FALSE(isSupportedForWriting(VerificationLevel::Experimental));
    CHECK_FALSE(isSupportedForWriting(VerificationLevel::Observed));
    CHECK(isSupportedForWriting(VerificationLevel::GoldenTested));
    CHECK(isSupportedForWriting(VerificationLevel::HardwareVerified));
}

TEST_CASE("no format currently claims write support", "[verification]") {
    // Phase stop condition: no mutating writer until the rules behind it are
    // verified. The checksum rule is now verified, but no serializer exists and
    // no round-trip test covers one, so writing stays Unsupported.
    CHECK(ka1::support().writing == VerificationLevel::Unsupported);
    CHECK(kaa::support().writing == VerificationLevel::Unsupported);
    CHECK_FALSE(isSupportedForWriting(ka1::support().writing));
    CHECK_FALSE(isSupportedForWriting(kaa::support().writing));
}

TEST_CASE("nothing claims HardwareVerified during this phase", "[verification]") {
    for (const auto& support : {ka1::support(), kaa::support()}) {
        CHECK(support.parsing < VerificationLevel::HardwareVerified);
        CHECK(support.writing < VerificationLevel::HardwareVerified);
        CHECK(support.conversion < VerificationLevel::HardwareVerified);
    }
}

TEST_CASE("every claimed level carries an evidence statement", "[verification]") {
    for (const auto& support : {ka1::support(), kaa::support()}) {
        CHECK_FALSE(support.format.empty());
        CHECK_FALSE(support.parsingEvidence.empty());
        CHECK_FALSE(support.writingEvidence.empty());
        CHECK_FALSE(support.conversionEvidence.empty());
    }
}

TEST_CASE("the worse validation state wins when observations combine", "[diagnostics]") {
    CHECK(worst(ValidationState::Valid, ValidationState::ChecksumMismatch)
          == ValidationState::ChecksumMismatch);
    CHECK(worst(ValidationState::ChecksumMismatch, ValidationState::Malformed)
          == ValidationState::Malformed);
    CHECK(worst(ValidationState::Truncated, ValidationState::StructurallyAmbiguous)
          == ValidationState::Truncated);
    // An ambiguous extent outranks an unrecognised variant: not knowing which
    // bytes belong to the patch is worse than not recognising what they mean.
    CHECK(worst(ValidationState::UnsupportedVariant, ValidationState::StructurallyAmbiguous)
          == ValidationState::StructurallyAmbiguous);
    CHECK(worst(ValidationState::Valid, ValidationState::Valid) == ValidationState::Valid);
}

TEST_CASE("a diagnostic message names the format, offset and both conditions",
          "[diagnostics]") {
    Diagnostic diagnostic;
    diagnostic.code = DiagnosticCode::PatchChecksumMismatch;
    diagnostic.severity = ValidationState::ChecksumMismatch;
    diagnostic.format = "KA1";
    diagnostic.offset = 0x2A;
    diagnostic.expected = 0x11;
    diagnostic.actual = 0x22;

    const std::string message = diagnostic.message();
    CHECK(message.find("KA1") != std::string::npos);
    CHECK(message.find("checksum") != std::string::npos);
    CHECK(message.find("2A") != std::string::npos);
    CHECK(message.find("17") != std::string::npos); // expected, decimal
    CHECK(message.find("34") != std::string::npos); // actual, decimal
}

TEST_CASE("a sink tracks the worst severity it has seen", "[diagnostics]") {
    DiagnosticSink sink;
    CHECK(sink.state() == ValidationState::Valid);
    CHECK(sink.empty());

    Diagnostic mild;
    mild.code = DiagnosticCode::PatchChecksumMismatch;
    mild.severity = ValidationState::ChecksumMismatch;
    sink.add(mild);
    CHECK(sink.state() == ValidationState::ChecksumMismatch);
    CHECK(sink.has(DiagnosticCode::PatchChecksumMismatch));

    Diagnostic severe;
    severe.code = DiagnosticCode::TruncatedCommonBlock;
    severe.severity = ValidationState::Truncated;
    sink.add(severe);
    CHECK(sink.state() == ValidationState::Truncated);

    // Adding a milder observation afterwards must not improve the verdict.
    sink.add(mild);
    CHECK(sink.state() == ValidationState::Truncated);
    CHECK(sink.entries().size() == 3);
}

TEST_CASE("validation state names are stable", "[diagnostics]") {
    CHECK(toString(ValidationState::Valid) == "Valid");
    CHECK(toString(ValidationState::Malformed) == "Malformed");
    CHECK(toString(ValidationState::Truncated) == "Truncated");
    CHECK(toString(ValidationState::ChecksumMismatch) == "ChecksumMismatch");
    CHECK(toString(ValidationState::UnsupportedVariant) == "UnsupportedVariant");
    CHECK(toString(ValidationState::StructurallyAmbiguous) == "StructurallyAmbiguous");
}
