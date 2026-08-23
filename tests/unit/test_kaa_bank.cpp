#include "Fixtures.h"

#include "k5000/kaa/Bank.h"

#include <catch2/catch_test_macros.hpp>

using namespace k5000;
using k5000::test::BankEntry;
using k5000::test::SourceSpec;

namespace {

const SourceSpec add{true, 0};
const SourceSpec pcm{false, 400};

} // namespace

TEST_CASE("a contiguous bank round-trips through extraction", "[kaa]") {
    const auto a = test::makeSingle("First   ", {add, pcm});
    const auto b = test::makeSingle("Second  ", {pcm, pcm});
    const auto c = test::makeSingle("Third   ", {add, add, add});

    const auto built = test::makeBank({{0, a, 0}, {1, b, 0}, {2, c, 0}});
    const auto parsed = kaa::parseBank(built.bytes);

    REQUIRE(parsed.usable);
    CHECK(parsed.bank.occupiedSlotCount() == 3);
    CHECK(parsed.bank.baseAddress == built.baseAddress);
    CHECK(parsed.bank.usedBytes() == a.size() + b.size() + c.size());

    const auto extraction = kaa::extractAll(parsed.bank, built.bytes);
    REQUIRE(extraction.patches.size() == 3);
    CHECK(extraction.failedSlots.empty());

    CHECK(extraction.patches[0].payload() == a);
    CHECK(extraction.patches[1].payload() == b);
    CHECK(extraction.patches[2].payload() == c);
    for (const auto& patch : extraction.patches) {
        CHECK(patch.single.validationState == ValidationState::Valid);
        CHECK(patch.payload().size() == patch.single.structuralSize);
    }
}

TEST_CASE("a sparse bank extracts only its occupied slots", "[kaa]") {
    const auto a = test::makeSingle("Slot1   ", {pcm, pcm});
    const auto b = test::makeSingle("Slot64  ", {add, pcm});
    const auto c = test::makeSingle("Slot128 ", {add, add});

    const auto built = test::makeBank({{0, a, 0}, {63, b, 0}, {127, c, 0}});
    const auto parsed = kaa::parseBank(built.bytes);
    REQUIRE(parsed.usable);
    CHECK(parsed.bank.occupiedSlotCount() == 3);

    const auto extraction = kaa::extractAll(parsed.bank, built.bytes);
    REQUIRE(extraction.patches.size() == 3);
    CHECK(extraction.patches[0].slot == 0);
    CHECK(extraction.patches[1].slot == 63);
    CHECK(extraction.patches[2].slot == 127);
    CHECK(extraction.patches[0].payload() == a);
    CHECK(extraction.patches[2].payload() == c);
}

// --------------------------------------------------------------------------
// Invariant A. This is the regression test the whole phase turns on.
// --------------------------------------------------------------------------

TEST_CASE("INVARIANT A: patch length never comes from the pointer delta", "[kaa][invariant]") {
    // A fragmented bank: 96 bytes of dead space sit between patch one and
    // patch two, exactly as in the corpus banks. A parser that computes
    // `size = next_pointer - pointer` would hand back a first patch that is
    // 96 bytes too long, with padding welded onto the end of the payload.
    constexpr std::size_t kGap = 96;

    const auto first = test::makeSingle("Fragment", {add, pcm});
    const auto second = test::makeSingle("AfterGap", {pcm, pcm});

    const auto built = test::makeBank({{0, first, 0}, {1, second, kGap}});
    const auto parsed = kaa::parseBank(built.bytes);
    REQUIRE(parsed.usable);

    // The hazard is real: the pointers really are further apart than the patch.
    const std::uint32_t pointerDelta =
        parsed.bank.slots[1].patchPointer - parsed.bank.slots[0].patchPointer;
    REQUIRE(pointerDelta == first.size() + kGap);
    REQUIRE(pointerDelta != first.size());

    const auto extraction = kaa::extractAll(parsed.bank, built.bytes);
    REQUIRE(extraction.patches.size() == 2);

    const auto& extracted = extraction.patches[0];
    CHECK(extracted.single.structuralSize == first.size());
    CHECK(extracted.payload().size() == first.size());
    CHECK(extracted.payload().size() != pointerDelta);
    CHECK(extracted.payload() == first);

    // The dead space really does sit between the two patches in the bank, so
    // the equality above is what rules the padding out - not the absence of a
    // particular byte value, which a 7-bit payload may legitimately contain.
    REQUIRE(built.offsets.size() == 2);
    const std::size_t endOfFirst = built.offsets[0] + first.size();
    CHECK(built.offsets[1] == endOfFirst + kGap);
    for (std::size_t i = 0; i < kGap; ++i) {
        CHECK(built.bytes[endOfFirst + i] == 0x7F);
    }

    CHECK(extraction.patches[1].payload() == second);
}

TEST_CASE("INVARIANT A: a stale ADD-kit pointer does not lengthen the patch", "[kaa][invariant]") {
    // A slot carrying an ADD wave kit pointer for a source that does not
    // exist - left behind by a larger patch that previously lived there.
    // 28 corpus slots look like this. Sizing from the pointer count would add
    // a phantom 806-byte wave kit to the payload.
    const auto payload = test::makeSingle("Stale   ", {pcm, pcm});
    REQUIRE(payload.size() == 254);

    const auto built = test::makeBank({{0, payload, 0}}, 0x00344E70, /*staleKitPointerSlot=*/0);
    const auto parsed = kaa::parseBank(built.bytes);
    REQUIRE(parsed.usable);

    // The stale entry is present and is surfaced as a diagnostic.
    CHECK(parsed.bank.slots[0].addWaveKitPointerCount() == 1);
    bool reported = false;
    for (const auto& d : parsed.bank.diagnostics) {
        reported = reported || d.code == DiagnosticCode::BankStaleAddKitPointer;
    }
    CHECK(reported);

    const auto extraction = kaa::extractAll(parsed.bank, built.bytes);
    REQUIRE(extraction.patches.size() == 1);

    // Sized from the descriptors, so the phantom kit is not included.
    CHECK(extraction.patches[0].single.addSourceCount == 0);
    CHECK(extraction.patches[0].single.structuralSize == 254);
    CHECK(extraction.patches[0].payload() == payload);
    CHECK(extraction.patches[0].payload().size() != 254 + ka1::kAddWaveKitSize);
}

TEST_CASE("extraction is byte preservation", "[kaa][preservation]") {
    // Deliberately imperfect input: the stored checksum is wrong. Extraction
    // must hand back exactly those bytes, wrong checksum included.
    auto payload = test::makeSingle("Preserve", {add, pcm});
    payload[ka1::kChecksumOffset] = static_cast<std::uint8_t>((payload[ka1::kChecksumOffset] + 3) & 0x7F);

    const auto built = test::makeBank({{5, payload, 0}});
    const auto parsed = kaa::parseBank(built.bytes);
    REQUIRE(parsed.usable);

    const auto extraction = kaa::extractAll(parsed.bank, built.bytes);
    REQUIRE(extraction.patches.size() == 1);

    const auto& extracted = extraction.patches[0];
    CHECK(extracted.payload() == payload);
    CHECK(extracted.single.validationState == ValidationState::ChecksumMismatch);
    CHECK_FALSE(extracted.single.checksumMatches());
    // The bad checksum byte survives untouched - no rewrite on the read path.
    CHECK(extracted.payload()[ka1::kChecksumOffset] == payload[ka1::kChecksumOffset]);
}

TEST_CASE("a bank of the wrong size is rejected", "[kaa][malformed]") {
    std::vector<std::uint8_t> bytes(kaa::kFileSize - 1, 0);
    const auto parsed = kaa::parseBank(bytes);

    CHECK_FALSE(parsed.usable);
    CHECK(parsed.bank.validationState == ValidationState::Malformed);
    bool reported = false;
    for (const auto& d : parsed.bank.diagnostics) {
        reported = reported || d.code == DiagnosticCode::BankUnexpectedFileSize;
    }
    CHECK(reported);
}

TEST_CASE("an empty bank is rejected rather than reported as valid", "[kaa][malformed]") {
    const std::vector<std::uint8_t> bytes(kaa::kFileSize, 0);
    const auto parsed = kaa::parseBank(bytes);

    CHECK_FALSE(parsed.usable);
    bool reported = false;
    for (const auto& d : parsed.bank.diagnostics) {
        reported = reported || d.code == DiagnosticCode::BankNoOccupiedSlots;
    }
    CHECK(reported);
}

TEST_CASE("a slot pointing outside the data region fails loudly", "[kaa][malformed]") {
    const auto payload = test::makeSingle("Valid   ", {pcm, pcm});
    auto built = test::makeBank({{0, payload, 0}, {1, payload, 0}});

    // Push slot 2 far past the end of the data region.
    const std::size_t row = 1 * kaa::kPointersPerSlot * 4;
    const std::uint32_t bad = built.baseAddress + static_cast<std::uint32_t>(kaa::kDataRegionSize) + 16;
    built.bytes[row] = static_cast<std::uint8_t>((bad >> 24) & 0xFF);
    built.bytes[row + 1] = static_cast<std::uint8_t>((bad >> 16) & 0xFF);
    built.bytes[row + 2] = static_cast<std::uint8_t>((bad >> 8) & 0xFF);
    built.bytes[row + 3] = static_cast<std::uint8_t>(bad & 0xFF);

    const auto parsed = kaa::parseBank(built.bytes);
    REQUIRE(parsed.usable);

    const auto extraction = kaa::extractAll(parsed.bank, built.bytes);
    CHECK(extraction.patches.size() == 1);
    REQUIRE(extraction.failedSlots.size() == 1);
    CHECK(extraction.failedSlots[0] == 1);
    CHECK(extraction.validationState == ValidationState::Malformed);
}

TEST_CASE("a slot whose patch is malformed is reported, not silently dropped", "[kaa][malformed]") {
    const auto good = test::makeSingle("Good    ", {pcm, pcm});
    auto bad = test::makeSingle("Bad     ", {pcm, pcm});
    bad[ka1::kSourceCountOffset] = 0; // impossible

    const auto built = test::makeBank({{0, good, 0}, {1, bad, 0}});
    const auto parsed = kaa::parseBank(built.bytes);
    REQUIRE(parsed.usable);

    const auto extraction = kaa::extractAll(parsed.bank, built.bytes);
    CHECK(extraction.patches.size() == 1);
    REQUIRE(extraction.failedSlots.size() == 1);
    CHECK(extraction.failedSlots[0] == 1);
    CHECK_FALSE(extraction.diagnostics.empty());
}

TEST_CASE("bank capacity is bounded by slots and by bytes", "[kaa][capacity]") {
    CHECK(kaa::kMaxPatches == 128);
    CHECK(kaa::kPatchDataBudget == 131072);
    CHECK(kaa::kFileSize == kaa::kHeaderSize + kaa::kDataRegionSize);
    CHECK(kaa::kFileSize == 134660);
    CHECK(kaa::kHeaderSize == 0x0E04);

    const auto payload = test::makeSingle("Budget  ", {add, add});
    const auto built = test::makeBank({{0, payload, 0}});
    const auto parsed = kaa::parseBank(built.bytes);
    REQUIRE(parsed.usable);
    CHECK(parsed.bank.usedBytes() == payload.size());
    CHECK(parsed.bank.freeBytes() == kaa::kPatchDataBudget - payload.size());
}
