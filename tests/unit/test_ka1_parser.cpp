#include "Fixtures.h"

#include "k5000/ka1/Parser.h"

#include <catch2/catch_test_macros.hpp>

using namespace k5000;
using k5000::test::SourceSpec;

namespace {

const SourceSpec add{true, 0};
const SourceSpec pcm{false, 400};

} // namespace

TEST_CASE("a two-source additive patch parses", "[ka1]") {
    const auto payload = test::makeSingle("PadWarm ", {add, add});
    const auto result = ka1::parse(payload);

    REQUIRE(result.structureKnown);
    CHECK(result.single.validationState == ValidationState::Valid);
    CHECK(result.single.name == "PadWarm ");
    CHECK(result.single.trimmedName() == "PadWarm");
    CHECK(result.single.sourceCount == 2);
    CHECK(result.single.addSourceCount == 2);
    CHECK(result.single.structuralSize == 1866);
    CHECK(result.single.structuralSize == payload.size());
    CHECK(result.single.checksumMatches());
    CHECK(result.single.rawPayload == payload);
}

TEST_CASE("a PCM-only patch parses and is the smallest possible", "[ka1]") {
    const auto payload = test::makeSingle("Bass    ", {pcm, pcm});
    const auto result = ka1::parse(payload);

    REQUIRE(result.structureKnown);
    CHECK(result.single.validationState == ValidationState::Valid);
    CHECK(result.single.addSourceCount == 0);
    CHECK(result.single.structuralSize == 254);
    for (const auto& source : result.single.sources) {
        CHECK(source.type == ka1::SourceType::Pcm);
    }
}

TEST_CASE("a mixed ADD/PCM patch classifies each source independently", "[ka1]") {
    const auto payload = test::makeSingle("Mixed   ", {add, pcm, add});
    const auto result = ka1::parse(payload);

    REQUIRE(result.structureKnown);
    CHECK(result.single.validationState == ValidationState::Valid);
    CHECK(result.single.sourceCount == 3);
    CHECK(result.single.addSourceCount == 2);
    CHECK(result.single.structuralSize == 1952);
    CHECK(result.single.sources[0].type == ka1::SourceType::Additive);
    CHECK(result.single.sources[1].type == ka1::SourceType::Pcm);
    CHECK(result.single.sources[2].type == ka1::SourceType::Additive);
    CHECK(result.single.sources[1].waveKit == 400);
    CHECK(result.single.sources[0].waveKit == ka1::kAdditiveWaveKit);
}

TEST_CASE("the largest patch is six additive sources", "[ka1]") {
    const auto payload = test::makeSingle("Maximum ", {add, add, add, add, add, add});
    const auto result = ka1::parse(payload);

    REQUIRE(result.structureKnown);
    CHECK(result.single.validationState == ValidationState::Valid);
    CHECK(result.single.structuralSize == 5434);
    CHECK(result.single.structuralSize == ka1::kMaxPatchSize);
}

TEST_CASE("the checksum covers common and sources but not the ADD wave kits", "[ka1][checksum]") {
    auto payload = test::makeSingle("Checksum", {add, pcm});
    const std::uint8_t original = payload[ka1::kChecksumOffset];

    SECTION("changing a common byte changes the checksum") {
        payload[0x30] = static_cast<std::uint8_t>(payload[0x30] ^ 0x01);
        test::refreshChecksum(payload);
        CHECK(payload[ka1::kChecksumOffset] != original);
    }

    SECTION("changing a source byte changes the checksum") {
        payload[ka1::sourceDescriptorOffset(0) + 3] ^= 0x01;
        test::refreshChecksum(payload);
        CHECK(payload[ka1::kChecksumOffset] != original);
    }

    SECTION("changing an ADD wave kit byte does NOT change the patch checksum") {
        const std::size_t kit = ka1::addWaveKitOffset(2, 0);
        payload[kit + 100] = static_cast<std::uint8_t>((payload[kit + 100] + 1) & 0x7F);
        test::refreshChecksum(payload);
        CHECK(payload[ka1::kChecksumOffset] == original);

        // ... but the kit's own checksum now fails, and that is reported.
        const auto result = ka1::parse(payload);
        CHECK(result.structureKnown);
        CHECK(result.single.checksumMatches());
        CHECK(result.single.validationState == ValidationState::ChecksumMismatch);
        bool reported = false;
        for (const auto& d : result.single.diagnostics) {
            reported = reported || d.code == DiagnosticCode::AddWaveKitChecksumMismatch;
        }
        CHECK(reported);
    }
}

TEST_CASE("a corrupted patch checksum is reported, never repaired", "[ka1][checksum]") {
    auto payload = test::makeSingle("Corrupt ", {add, add});
    const auto expected = payload[ka1::kChecksumOffset];
    payload[ka1::kChecksumOffset] = static_cast<std::uint8_t>((expected + 1) & 0x7F);

    const auto result = ka1::parse(payload);

    REQUIRE(result.structureKnown);
    CHECK(result.single.validationState == ValidationState::ChecksumMismatch);
    CHECK(result.single.checksumStored == ((expected + 1) & 0x7F));
    CHECK(result.single.checksumCalculated == expected);
    CHECK_FALSE(result.single.checksumMatches());

    // The payload is preserved exactly as it was read - no repair.
    CHECK(result.single.rawPayload == payload);
    CHECK(result.single.rawPayload[ka1::kChecksumOffset] != result.single.checksumCalculated);
}

TEST_CASE("truncation is detected at every structural stage", "[ka1][malformed]") {
    const auto full = test::makeSingle("Truncate", {add, pcm});

    SECTION("truncated common block") {
        const std::vector<std::uint8_t> cut(full.begin(), full.begin() + 40);
        const auto result = ka1::parse(cut);
        CHECK_FALSE(result.structureKnown);
        CHECK(result.single.validationState == ValidationState::Truncated);
    }

    SECTION("truncated source descriptors") {
        const std::vector<std::uint8_t> cut(full.begin(), full.begin() + 120);
        const auto result = ka1::parse(cut);
        CHECK_FALSE(result.structureKnown);
        CHECK(result.single.validationState == ValidationState::Truncated);
        CHECK(result.single.sourceCount == 2);
    }

    SECTION("truncated ADD wave kit") {
        const std::vector<std::uint8_t> cut(full.begin(), full.end() - 10);
        const auto result = ka1::parse(cut);
        CHECK_FALSE(result.structureKnown);
        CHECK(result.single.validationState == ValidationState::Truncated);
    }

    SECTION("empty input") {
        const auto result = ka1::parse({});
        CHECK_FALSE(result.structureKnown);
        CHECK(result.single.validationState == ValidationState::Truncated);
    }
}

TEST_CASE("an impossible source count is rejected without reading further", "[ka1][malformed]") {
    auto payload = test::makeSingle("BadCount", {add, add});

    SECTION("zero sources") {
        payload[ka1::kSourceCountOffset] = 0;
        const auto result = ka1::parse(payload);
        CHECK_FALSE(result.structureKnown);
        CHECK(result.single.validationState == ValidationState::Malformed);
    }

    SECTION("127 sources, as seen in the corpus") {
        payload[ka1::kSourceCountOffset] = 127;
        const auto result = ka1::parse(payload);
        CHECK_FALSE(result.structureKnown);
        CHECK(result.single.validationState == ValidationState::Malformed);
        bool reported = false;
        for (const auto& d : result.single.diagnostics) {
            reported = reported || d.code == DiagnosticCode::SourceCountOutOfRange;
        }
        CHECK(reported);
    }
}

TEST_CASE("trailing bytes are reported, not trimmed away silently", "[ka1][malformed]") {
    auto payload = test::makeSingle("Trailing", {add, pcm});
    const std::size_t structural = payload.size();
    payload.insert(payload.end(), {0x01, 0x02}); // the corpus "+2 bytes" pattern

    const auto result = ka1::parse(payload);

    REQUIRE(result.structureKnown);
    CHECK(result.single.structuralSize == structural);
    CHECK(result.single.validationState == ValidationState::Malformed);
    CHECK(result.single.rawPayload.size() == structural); // extra bytes are not adopted
    bool reported = false;
    for (const auto& d : result.single.diagnostics) {
        reported = reported || d.code == DiagnosticCode::TrailingBytes;
    }
    CHECK(reported);
}

TEST_CASE("length and descriptors disagreeing is ambiguous, not a guess", "[ka1][malformed]") {
    // Reproduces the one corpus file where the file length implies three ADD
    // sources while the wave kit numbers imply two. Neither reading may win.
    auto payload = test::makeSingle("Ambiguou", {add, add, add, pcm});
    const std::size_t waveOffset =
        ka1::sourceDescriptorOffset(2) + ka1::kWaveKitOffsetInSource;
    payload[waveOffset] = 0x3F; // 8186, out of range - as in the corpus
    payload[waveOffset + 1] = 0x7A;
    test::refreshChecksum(payload);

    const auto result = ka1::parse(payload);

    REQUIRE(result.structureKnown);
    CHECK(result.single.addSourceCount == 2);
    CHECK(result.single.validationState == ValidationState::StructurallyAmbiguous);
    CHECK(result.single.checksumMatches()); // self-consistent, which is the trap

    bool ambiguity = false;
    bool waveRange = false;
    for (const auto& d : result.single.diagnostics) {
        ambiguity = ambiguity || d.code == DiagnosticCode::LengthConflictsWithSourceDescriptors;
        waveRange = waveRange || d.code == DiagnosticCode::WaveKitNumberOutOfRange;
    }
    CHECK(ambiguity);
    CHECK(waveRange);
}

TEST_CASE("embedded parsing accepts trailing container data", "[ka1]") {
    auto payload = test::makeSingle("Embedded", {add, pcm});
    const std::size_t structural = payload.size();
    payload.insert(payload.end(), 500, 0x7F); // neighbouring bank bytes

    ka1::ParseOptions options;
    options.embedded = true;
    const auto result = ka1::parse(payload, options);

    REQUIRE(result.structureKnown);
    CHECK(result.single.validationState == ValidationState::Valid);
    CHECK(result.single.structuralSize == structural);
    CHECK(result.single.rawPayload.size() == structural);
}

TEST_CASE("the checksum helper matches the documented formula", "[ka1][checksum]") {
    const auto payload = test::makeSingle("Formula ", {pcm, pcm});
    std::uint32_t total = ka1::kChecksumSeed;
    for (std::size_t i = 1; i < ka1::patchSize(2, 0); ++i) {
        total += payload[i];
    }
    CHECK(ka1::computeChecksum(payload, 2) == static_cast<std::uint8_t>(total & 0x7F));
}
