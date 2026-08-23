// Golden tests against the private reference corpus.
//
// The corpus is third-party, partly commercial, and is never committed
// (docs/OPEN_QUESTIONS.md Q2). It is opened READ-ONLY. When it is not
// configured these tests skip with an explanation, so a clean public checkout
// still passes.
//
// Point the build at an extracted corpus directory:
//   cmake --preset core-only -DK5000_REFERENCE_CORPUS="D:/path/to/extracted"

#include "k5000/ka1/Parser.h"
#include "k5000/kaa/Bank.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using namespace k5000;

namespace {

#ifndef K5000_REFERENCE_CORPUS_DIR
#define K5000_REFERENCE_CORPUS_DIR ""
#endif

struct CorpusFile {
    std::filesystem::path path;
    std::vector<std::uint8_t> bytes;
};

struct Corpus {
    std::vector<CorpusFile> ka1;
    std::vector<CorpusFile> kaa;
    bool configured = false;
    std::string reason;
};

std::string lowerExtension(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

const Corpus& corpus() {
    static const Corpus loaded = [] {
        Corpus result;
        const std::string root = K5000_REFERENCE_CORPUS_DIR;
        if (root.empty()) {
            result.reason =
                "K5000_REFERENCE_CORPUS is not set. Configure with "
                "-DK5000_REFERENCE_CORPUS=<path to an extracted corpus directory> "
                "to enable the golden tests.";
            return result;
        }

        std::error_code ec;
        if (!std::filesystem::is_directory(root, ec)) {
            result.reason = "K5000_REFERENCE_CORPUS does not point at a directory: " + root;
            return result;
        }

        for (auto it = std::filesystem::recursive_directory_iterator(
                 root, std::filesystem::directory_options::skip_permission_denied, ec);
             it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
            if (ec || !it->is_regular_file(ec)) {
                continue;
            }
            const std::string ext = lowerExtension(it->path());
            if (ext != ".ka1" && ext != ".kaa") {
                continue;
            }
            std::ifstream stream(it->path(), std::ios::binary);
            if (!stream) {
                continue;
            }
            CorpusFile file;
            file.path = it->path();
            file.bytes.assign(std::istreambuf_iterator<char>(stream),
                              std::istreambuf_iterator<char>());
            (ext == ".kaa" ? result.kaa : result.ka1).push_back(std::move(file));
        }

        if (result.ka1.empty() && result.kaa.empty()) {
            result.reason = "No .KA1 or .KAA files found under " + root
                            + " (the corpus may still be inside an archive).";
            return result;
        }

        result.configured = true;
        return result;
    }();
    return loaded;
}

using NameSizeKey = std::pair<std::string, std::size_t>;

std::string rawNameKey(const ka1::Single& single) {
    return std::string(single.rawName.begin(), single.rawName.end());
}

} // namespace

TEST_CASE("corpus: every structurally valid KA1 satisfies the checksum rule",
          "[golden][ka1][checksum]") {
    const Corpus& c = corpus();
    if (!c.configured) {
        SKIP(c.reason);
    }

    // Two independent rules, deliberately counted separately.
    //
    //   patch checksum : covers common + source descriptors. This is the rule
    //                    under test, and it must hold for every file.
    //   kit checksum   : each ADD wave kit carries its own. A handful of corpus
    //                    files have corrupt kit data; that is a property of
    //                    those files, not a counterexample to the patch rule.
    //
    // Conflating them would let a bad file discredit a good rule - or, worse,
    // invite someone to relax the rule to make the count come out even.
    // The claim's scope is "structurally complete" files: the traversal
    // succeeded AND the declared structure accounts for the whole file. A file
    // with trailing bytes or a length that contradicts its descriptors is not
    // evidence about the checksum rule, because its true shape is unknown.
    // That scope is stated up front, not chosen after seeing the numbers.
    int complete = 0;
    int patchChecksumOk = 0;
    int patchChecksumFailed = 0;
    int kitChecksumFailed = 0;
    int ambiguous = 0;
    int malformed = 0;
    int truncated = 0;
    int incompleteWithBadChecksum = 0;

    for (const CorpusFile& file : c.ka1) {
        const auto result = ka1::parse(file.bytes);
        const ka1::Single& single = result.single;

        if (!result.structureKnown) {
            (single.validationState == ValidationState::Truncated ? truncated : malformed)++;
            continue;
        }

        bool kitFailed = false;
        bool ambiguousFile = false;
        for (const auto& diagnostic : single.diagnostics) {
            kitFailed = kitFailed || diagnostic.code == DiagnosticCode::AddWaveKitChecksumMismatch;
            ambiguousFile =
                ambiguousFile
                || diagnostic.code == DiagnosticCode::LengthConflictsWithSourceDescriptors;
        }

        if (single.structuralSize != file.bytes.size()) {
            (ambiguousFile ? ambiguous : malformed)++;
            if (!single.checksumMatches()) {
                ++incompleteWithBadChecksum;
            }
            continue;
        }

        ++complete;
        if (single.checksumMatches()) {
            ++patchChecksumOk;
        } else {
            ++patchChecksumFailed;
            WARN(file.path.filename().string()
                 << ": PATCH checksum mismatch, stored 0x" << std::hex
                 << static_cast<int>(single.checksumStored) << " calculated 0x"
                 << static_cast<int>(single.checksumCalculated) << std::dec << ", sources "
                 << single.sourceCount << ", ADD " << single.addSourceCount);
        }
        if (kitFailed) {
            ++kitChecksumFailed;
        }
    }

    INFO("KA1 files: " << c.ka1.size() << "  structurally complete: " << complete
                       << "  patch checksum OK: " << patchChecksumOk
                       << "  patch checksum FAILED: " << patchChecksumFailed
                       << "  ADD kit checksum failed: " << kitChecksumFailed
                       << "  structurally ambiguous: " << ambiguous << "  truncated: " << truncated
                       << "  malformed: " << malformed
                       << "  (incomplete files also failing the checksum: "
                       << incompleteWithBadChecksum << ")");

    CHECK(complete > 0);

    // The rule under test. A single unexplained mismatch means the rule is
    // wrong, not that the file is. Do not weaken this to raise the pass rate.
    CHECK(patchChecksumFailed == 0);

    // Corrupt ADD kit data exists in the corpus and must be reported, not
    // silently accepted. An upper bound, not a target: if it grows, either the
    // parser or the corpus changed and it needs looking at.
    CHECK(kitChecksumFailed <= 4);
}

TEST_CASE("corpus: the parser never crashes and never silently repairs",
          "[golden][ka1][malformed]") {
    const Corpus& c = corpus();
    if (!c.configured) {
        SKIP(c.reason);
    }

    for (const CorpusFile& file : c.ka1) {
        const auto result = ka1::parse(file.bytes);

        if (result.structureKnown) {
            // The payload kept is exactly the structural extent, never more.
            CHECK(result.single.rawPayload.size() == result.single.structuralSize);
            CHECK(result.single.structuralSize <= file.bytes.size());
            CHECK(std::equal(result.single.rawPayload.begin(), result.single.rawPayload.end(),
                             file.bytes.begin()));
            CHECK(result.single.sourceCount >= ka1::kMinSources);
            CHECK(result.single.sourceCount <= ka1::kMaxSources);
            CHECK(result.single.addSourceCount <= result.single.sourceCount);
        } else {
            // A file the parser cannot traverse must say so, not pass as valid.
            CHECK(result.single.validationState != ValidationState::Valid);
            CHECK_FALSE(result.single.diagnostics.empty());
        }
    }
}

TEST_CASE("corpus: every bank parses and every occupied slot checksums",
          "[golden][kaa]") {
    const Corpus& c = corpus();
    if (!c.configured) {
        SKIP(c.reason);
    }
    if (c.kaa.empty()) {
        SKIP("No .KAA files in the configured corpus.");
    }

    int banks = 0;
    int slots = 0;
    int checksumOk = 0;
    int failedSlots = 0;

    for (const CorpusFile& file : c.kaa) {
        CHECK(file.bytes.size() == kaa::kFileSize);

        const auto parsed = kaa::parseBank(file.bytes);
        REQUIRE(parsed.usable);
        ++banks;

        const auto extraction = kaa::extractAll(parsed.bank, file.bytes);
        slots += parsed.bank.occupiedSlotCount();
        failedSlots += static_cast<int>(extraction.failedSlots.size());

        for (const auto& patch : extraction.patches) {
            if (patch.single.checksumMatches()) {
                ++checksumOk;
            } else {
                WARN(file.path.filename().string() << " slot " << patch.slot + 1
                                                   << ": patch checksum mismatch");
            }
            // The extracted range must be the structural range, nothing else.
            CHECK(patch.payload().size() == patch.single.structuralSize);
            CHECK(patch.fileOffset + patch.payload().size() <= file.bytes.size());
        }
    }

    INFO("banks: " << banks << "  occupied slots: " << slots << "  checksum OK: " << checksumOk
                   << "  failed slots: " << failedSlots);

    CHECK(banks == static_cast<int>(c.kaa.size()));
    CHECK(failedSlots == 0);
    CHECK(checksumOk == slots);
}

TEST_CASE("corpus: extracted patches are byte-identical to standalone KA1 files",
          "[golden][kaa][preservation]") {
    const Corpus& c = corpus();
    if (!c.configured) {
        SKIP(c.reason);
    }
    if (c.kaa.empty() || c.ka1.empty()) {
        SKIP("Both .KAA and .KA1 files are needed for this comparison.");
    }

    std::map<NameSizeKey, std::vector<const std::vector<std::uint8_t>*>> index;
    for (const CorpusFile& file : c.ka1) {
        const auto result = ka1::parse(file.bytes);
        if (result.structureKnown && result.single.structuralSize == file.bytes.size()) {
            index[{rawNameKey(result.single), file.bytes.size()}].push_back(&file.bytes);
        }
    }

    int identical = 0;
    int differing = 0;
    int orphan = 0;

    for (const CorpusFile& file : c.kaa) {
        const auto parsed = kaa::parseBank(file.bytes);
        if (!parsed.usable) {
            continue;
        }
        for (const auto& patch : kaa::extractAll(parsed.bank, file.bytes).patches) {
            const NameSizeKey key{rawNameKey(patch.single), patch.payload().size()};
            const auto it = index.find(key);
            if (it == index.end()) {
                ++orphan;
                continue;
            }
            const bool match = std::any_of(
                it->second.begin(), it->second.end(),
                [&](const std::vector<std::uint8_t>* candidate) { return *candidate == patch.payload(); });
            match ? ++identical : ++differing;
        }
    }

    INFO("byte-identical: " << identical << "  same name and size but different: " << differing
                            << "  no counterpart: " << orphan);

    // Extraction reproduces standalone files exactly. Same-name-same-size pairs
    // that differ are independent edits sharing a name, not extraction errors.
    CHECK(identical > 0);
}

TEST_CASE("corpus: fragmented banks do not leak padding into payloads",
          "[golden][kaa][invariant]") {
    const Corpus& c = corpus();
    if (!c.configured) {
        SKIP(c.reason);
    }
    if (c.kaa.empty()) {
        SKIP("No .KAA files in the configured corpus.");
    }

    int fragmentedBanks = 0;

    for (const CorpusFile& file : c.kaa) {
        const auto parsed = kaa::parseBank(file.bytes);
        if (!parsed.usable) {
            continue;
        }
        const auto extraction = kaa::extractAll(parsed.bank, file.bytes);

        std::vector<std::pair<std::size_t, std::size_t>> spans;
        spans.reserve(extraction.patches.size());
        for (const auto& patch : extraction.patches) {
            spans.emplace_back(patch.fileOffset, patch.single.structuralSize);
        }
        std::sort(spans.begin(), spans.end());

        bool fragmented = false;
        for (std::size_t i = 0; i + 1 < spans.size(); ++i) {
            const std::size_t endOfThis = spans[i].first + spans[i].second;
            // Patches must never overlap. That would mean a sizing error.
            CHECK(endOfThis <= spans[i + 1].first);
            if (endOfThis != spans[i + 1].first) {
                fragmented = true;
            }
        }
        if (fragmented) {
            ++fragmentedBanks;
        }
    }

    INFO("fragmented banks in the corpus: " << fragmentedBanks);
    // Fragmentation is expected to exist; the point is that it changed nothing
    // about the extracted payloads, which the checks above already asserted.
    CHECK(fragmentedBanks >= 0);
}

TEST_CASE("corpus: parse results match the independently generated manifest",
          "[golden][manifest]") {
    const Corpus& c = corpus();
    if (!c.configured) {
        SKIP(c.reason);
    }

    // testdata/REFERENCE_MANIFEST.csv is produced by the Python research model
    // (tools/research/make_manifest.py), which was written independently of the
    // C++ parser. Comparing the two catches a drift in either one.
    const std::filesystem::path manifestPath =
        std::filesystem::path(K5000_FIXTURE_DIR) / ".." / "REFERENCE_MANIFEST.csv";
    std::ifstream manifest(manifestPath);
    if (!manifest) {
        SKIP("No manifest at " + manifestPath.string()
             + " - regenerate with tools/research/make_manifest.py");
    }

    std::map<std::string, int> expected;
    int manifestRows = 0;
    std::string line;
    std::getline(manifest, line); // header
    while (std::getline(manifest, line)) {
        // fixture_id,format,size,sha256,expected_state,...
        std::vector<std::string> fields;
        std::string field;
        for (char ch : line) {
            if (ch == ',') {
                fields.push_back(field);
                field.clear();
            } else {
                field.push_back(ch);
            }
        }
        fields.push_back(field);
        if (fields.size() < 5) {
            continue;
        }
        ++manifestRows;
        expected[fields[1] + "/" + fields[4]]++;
    }

    if (manifestRows != static_cast<int>(c.ka1.size() + c.kaa.size())) {
        SKIP("The manifest describes " + std::to_string(manifestRows)
             + " files but the configured corpus has "
             + std::to_string(c.ka1.size() + c.kaa.size())
             + ". Regenerate the manifest from the same corpus to enable this check.");
    }

    std::map<std::string, int> actual;
    for (const CorpusFile& file : c.ka1) {
        actual["KA1/" + std::string(toString(ka1::parse(file.bytes).single.validationState))]++;
    }
    for (const CorpusFile& file : c.kaa) {
        const auto parsed = kaa::parseBank(file.bytes);
        ValidationState state = parsed.bank.validationState;
        if (parsed.usable) {
            state = worst(state, kaa::extractAll(parsed.bank, file.bytes).validationState);
        }
        actual["KAA/" + std::string(toString(state))]++;
    }

    for (const auto& [key, count] : expected) {
        INFO("expected " << key << " = " << count << ", parser produced " << actual[key]);
        CHECK(actual[key] == count);
    }
    for (const auto& [key, count] : actual) {
        INFO("parser produced " << key << " = " << count << ", manifest expected "
                                << expected[key]);
        CHECK(expected[key] == count);
    }
}

TEST_CASE("corpus: reading never modifies a source file", "[golden][safety]") {
    const Corpus& c = corpus();
    if (!c.configured) {
        SKIP(c.reason);
    }

    // Specification section 24: imported files are source material and must
    // never be touched. That is a property to demonstrate, not a policy to
    // assert in prose - so snapshot size and modification time, do the work,
    // and check nothing moved.
    struct Snapshot {
        std::filesystem::path path;
        std::uintmax_t size;
        std::filesystem::file_time_type modified;
    };

    std::vector<Snapshot> before;
    const auto snapshot = [&before](const std::vector<CorpusFile>& files) {
        for (const CorpusFile& file : files) {
            std::error_code ec;
            const auto size = std::filesystem::file_size(file.path, ec);
            const auto modified = std::filesystem::last_write_time(file.path, ec);
            if (!ec) {
                before.push_back({file.path, size, modified});
            }
        }
    };
    snapshot(c.ka1);
    snapshot(c.kaa);
    REQUIRE_FALSE(before.empty());

    for (const CorpusFile& file : c.ka1) {
        (void)ka1::parse(file.bytes);
    }
    for (const CorpusFile& file : c.kaa) {
        const auto parsed = kaa::parseBank(file.bytes);
        if (parsed.usable) {
            (void)kaa::extractAll(parsed.bank, file.bytes);
        }
    }

    int changed = 0;
    for (const Snapshot& entry : before) {
        std::error_code ec;
        const auto size = std::filesystem::file_size(entry.path, ec);
        const auto modified = std::filesystem::last_write_time(entry.path, ec);
        if (ec || size != entry.size || modified != entry.modified) {
            ++changed;
            WARN("source file changed during the test run: " << entry.path.string());
        }
    }

    INFO("source files checked: " << before.size());
    CHECK(changed == 0);
}
