// k5000cli - diagnostic tool over the same core libraries as the GUI.
//
// Read-only with one exception: `extract` writes new files, and never
// overwrites an existing one. Source files are never modified.

#include "k5000/ka1/Parser.h"
#include "k5000/kaa/Bank.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::vector<std::uint8_t> readFile(const std::filesystem::path& path, bool& ok) {
    std::ifstream stream(path, std::ios::binary);
    ok = static_cast<bool>(stream);
    if (!ok) {
        return {};
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(stream),
                                     std::istreambuf_iterator<char>());
}

void printDiagnostics(const std::vector<k5000::Diagnostic>& diagnostics,
                      const std::filesystem::path& source) {
    for (const auto& diagnostic : diagnostics) {
        std::cout << "  [" << k5000::toString(diagnostic.severity) << "] "
                  << source.filename().string() << ": " << diagnostic.message() << '\n';
    }
}

void printSupport(const k5000::FormatSupport& s) {
    std::cout << "  " << std::left << std::setw(6) << s.format
              << " parsing=" << std::setw(17) << k5000::toString(s.parsing)
              << " writing=" << std::setw(13) << k5000::toString(s.writing)
              << " conversion=" << k5000::toString(s.conversion) << '\n';
}

int commandFormats() {
    std::cout << "Format support (docs/OPEN_QUESTIONS.md Q4).\n"
                 "Write paths below GoldenTested are disabled.\n\n";
    printSupport(k5000::ka1::support());
    printSupport(k5000::kaa::support());
    std::cout << "\nEvidence:\n"
              << "  KA1 parsing: " << k5000::ka1::support().parsingEvidence << '\n'
              << "  KAA parsing: " << k5000::kaa::support().parsingEvidence << '\n';
    return 0;
}

int commandInfoKa1(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
    const auto result = k5000::ka1::parse(bytes);
    const auto& single = result.single;

    std::cout << path.filename().string() << '\n'
              << "  state           : " << k5000::toString(single.validationState) << '\n'
              << "  name            : \"" << single.name << "\"\n"
              << "  sources         : " << single.sourceCount << " (" << single.addSourceCount
              << " ADD, " << single.sourceCount - single.addSourceCount << " PCM)\n";

    if (result.structureKnown) {
        std::cout << "  structural size : " << single.structuralSize << " bytes\n"
                  << "  file size       : " << bytes.size() << " bytes\n"
                  << "  checksum        : stored 0x" << std::hex << std::uppercase
                  << static_cast<int>(single.checksumStored) << ", calculated 0x"
                  << static_cast<int>(single.checksumCalculated) << std::dec << std::nouppercase
                  << (single.checksumMatches() ? "  (match)" : "  (MISMATCH)") << '\n';
        for (const auto& source : single.sources) {
            std::cout << "    source " << source.index + 1 << ": "
                      << k5000::ka1::toString(source.type) << ", wave kit " << source.waveKit
                      << '\n';
        }
    }

    printDiagnostics(single.diagnostics, path);
    return single.validationState == k5000::ValidationState::Valid ? 0 : 1;
}

int commandInfoKaa(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
    const auto parsed = k5000::kaa::parseBank(bytes);
    const auto& bank = parsed.bank;

    std::cout << path.filename().string() << '\n'
              << "  state       : " << k5000::toString(bank.validationState) << '\n';
    if (!parsed.usable) {
        printDiagnostics(bank.diagnostics, path);
        return 1;
    }

    std::cout << "  base address: 0x" << std::hex << std::uppercase << bank.baseAddress << std::dec
              << std::nouppercase << '\n'
              << "  occupied    : " << bank.occupiedSlotCount() << " / " << k5000::kaa::kSlotCount
              << " slots\n"
              << "  used        : " << bank.usedBytes() << " / " << k5000::kaa::kPatchDataBudget
              << " bytes (" << bank.usedBytes() * 100 / k5000::kaa::kPatchDataBudget << "%)\n";

    const auto extraction = k5000::kaa::extractAll(bank, bytes);
    std::cout << "  extracted   : " << extraction.patches.size() << " patches";
    if (!extraction.failedSlots.empty()) {
        std::cout << ", " << extraction.failedSlots.size() << " slots FAILED";
    }
    std::cout << '\n';

    for (const auto& patch : extraction.patches) {
        std::cout << "    " << std::setw(3) << patch.slot + 1 << "  \"" << patch.single.name
                  << "\"  " << std::setw(5) << patch.single.structuralSize << " bytes  "
                  << patch.single.sourceCount << " src / " << patch.single.addSourceCount
                  << " ADD  " << k5000::toString(patch.single.validationState) << '\n';
    }

    printDiagnostics(bank.diagnostics, path);
    printDiagnostics(extraction.diagnostics, path);
    return extraction.failedSlots.empty() ? 0 : 1;
}

std::string sanitize(const std::string& name) {
    std::string out;
    for (char c : name) {
        const bool invalid = c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\'
                             || c == '|' || c == '?' || c == '*' || static_cast<unsigned char>(c) < 0x20;
        out.push_back(invalid ? '_' : c);
    }
    while (!out.empty() && (out.back() == ' ' || out.back() == '.')) {
        out.pop_back();
    }
    return out.empty() ? std::string("unnamed") : out;
}

int commandExtract(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes,
                   const std::filesystem::path& outDir) {
    const auto parsed = k5000::kaa::parseBank(bytes);
    if (!parsed.usable) {
        std::cerr << "not a readable KAA bank\n";
        printDiagnostics(parsed.bank.diagnostics, path);
        return 1;
    }

    std::error_code ec;
    std::filesystem::create_directories(outDir, ec);
    if (ec) {
        std::cerr << "cannot create output directory: " << ec.message() << '\n';
        return 1;
    }

    const auto extraction = k5000::kaa::extractAll(parsed.bank, bytes);
    int written = 0;
    int skipped = 0;

    for (const auto& patch : extraction.patches) {
        char prefix[8];
        std::snprintf(prefix, sizeof(prefix), "A%03d_", patch.slot + 1);
        const auto target = outDir / (prefix + sanitize(patch.single.trimmedName()) + ".KA1");

        if (std::filesystem::exists(target)) {
            std::cerr << "  refusing to overwrite " << target.filename().string() << '\n';
            ++skipped;
            continue;
        }

        // Atomic-ish: write a temporary, then rename into place.
        const auto temporary = target.string() + ".partial";
        {
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            const auto& payload = patch.payload();
            out.write(reinterpret_cast<const char*>(payload.data()),
                      static_cast<std::streamsize>(payload.size()));
            if (!out) {
                std::cerr << "  write failed for " << target.filename().string() << '\n';
                out.close();
                std::filesystem::remove(temporary, ec);
                ++skipped;
                continue;
            }
        }
        std::filesystem::rename(temporary, target, ec);
        if (ec) {
            std::cerr << "  rename failed for " << target.filename().string() << '\n';
            std::filesystem::remove(temporary, ec);
            ++skipped;
            continue;
        }
        ++written;
    }

    std::cout << "extracted " << written << " patches to " << outDir.string() << '\n';
    if (skipped != 0) {
        std::cout << skipped << " skipped\n";
    }
    if (!extraction.failedSlots.empty()) {
        std::cout << extraction.failedSlots.size() << " occupied slots could not be read:\n";
        printDiagnostics(extraction.diagnostics, path);
    }
    return (skipped == 0 && extraction.failedSlots.empty()) ? 0 : 1;
}

int usage() {
    std::cerr << "k5000cli - K5000 Librarian diagnostic tool\n\n"
                 "  k5000cli formats\n"
                 "  k5000cli info <file.KA1|file.KAA>\n"
                 "  k5000cli extract <file.KAA> <output-directory>\n\n"
                 "Input files are never modified. Extraction never overwrites.\n";
    return 2;
}

} // namespace

int main(int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) {
        return usage();
    }

    if (args[0] == "formats") {
        return commandFormats();
    }

    if ((args[0] == "info" && args.size() == 2)
        || (args[0] == "extract" && args.size() == 3)) {
        const std::filesystem::path path(args[1]);
        bool ok = false;
        const auto bytes = readFile(path, ok);
        if (!ok) {
            std::cerr << "cannot read " << path.string() << '\n';
            return 1;
        }

        if (args[0] == "extract") {
            return commandExtract(path, bytes, std::filesystem::path(args[2]));
        }
        return bytes.size() == k5000::kaa::kFileSize ? commandInfoKaa(path, bytes)
                                                     : commandInfoKa1(path, bytes);
    }

    return usage();
}
