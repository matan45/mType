#include "FormattingCorpusTestSuite.hpp"
#include "FormattingTestHelpers.hpp"
#include "../src/utils/MemoryFileReader.hpp"
#include "../../mType/diagnostics/SourceFileCache.hpp"
#include "../../mType/lexer/Lexer.hpp"
#include "../../mType/services/FileReader.hpp"

#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mtype::lsp::test {

namespace {

namespace fs = std::filesystem;

constexpr std::size_t kMinimumCorpusFiles = 250;
constexpr std::size_t kMaxReportedFailures = 25;
const std::string kLexerBufferKey = "mtype-formatting-corpus://buffer.mt";

// Folders under mType/tests/testFiles whose non-error sources form the corpus:
// the standard library plus features rich in strings, comments, interpolation
// and keyword-prefixed identifiers (forEach, finally, format...).
const char* const kCorpusDirectories[] = {"lib", "stream", "controlFlow", "interpolation"};

struct CorpusToken {
    token::TokenType type;
    std::string text;
    std::int64_t intValue;
    double floatValue;

    auto operator<=>(const CorpusToken&) const = default;
};

using TokenList = std::vector<CorpusToken>;

struct LexResult {
    bool ok = true;
    TokenList tokens;
    std::string error;
};

struct CorpusReport {
    std::size_t tokenChecked = 0;
    std::size_t skippedUnlexable = 0;
    std::vector<std::string> failures;
};

// Lex `text` with the real mType lexer. Its constructor publishes the buffer
// into the process-wide SourceFileCache, so the entry is dropped afterwards.
LexResult lexAll(const std::string& text) {
    struct CacheGuard {
        ~CacheGuard() { diagnostics::SourceFileCache::instance().invalidate(kLexerBufferKey); }
    } guard;

    LexResult result;
    try {
        auto reader = std::make_unique<MemoryFileReader>();
        reader->setContent(kLexerBufferKey, text);
        lexer::Lexer lex(kLexerBufferKey, std::move(reader));
        for (;;) {
            token::Token next = lex.getNextToken();
            // stringValue is a view into the lexer's buffers: copy it now.
            result.tokens.push_back({next.type, std::string(next.stringValue),
                                     next.intValue, next.floatValue});
            if (next.type == token::TokenType::END) break;
        }
    } catch (const std::exception& e) {
        result.ok = false;
        result.error = e.what();
    } catch (...) {
        result.ok = false;
        result.error = "non-standard exception";
    }
    return result;
}

// organizeImports may legitimately reorder and de-duplicate the leading import
// statements, so those compare as a sorted set; everything after them must
// match token for token.
struct SplitTokens {
    std::vector<TokenList> imports;
    TokenList rest;
};

SplitTokens splitLeadingImports(const TokenList& tokens) {
    SplitTokens split;
    std::size_t i = 0;
    while (i < tokens.size() && tokens[i].type == token::TokenType::IMPORT) {
        std::size_t end = i + 1;
        while (end < tokens.size() && tokens[end].type != token::TokenType::SEMICOLON
               && tokens[end].type != token::TokenType::END) {
            ++end;
        }
        if (end < tokens.size() && tokens[end].type == token::TokenType::SEMICOLON) ++end;
        split.imports.emplace_back(tokens.begin() + static_cast<std::ptrdiff_t>(i),
                                   tokens.begin() + static_cast<std::ptrdiff_t>(end));
        i = end;
    }
    split.rest.assign(tokens.begin() + static_cast<std::ptrdiff_t>(i), tokens.end());
    std::sort(split.imports.begin(), split.imports.end());
    split.imports.erase(std::unique(split.imports.begin(), split.imports.end()), split.imports.end());
    return split;
}

std::string describeFirstDifference(const TokenList& before, const TokenList& after) {
    std::size_t i = 0;
    while (i < before.size() && i < after.size() && before[i] == after[i]) ++i;
    auto show = [](const TokenList& list, std::size_t index) {
        return index < list.size() ? formatting::visible(list[index].text) : std::string("<end>");
    };
    return "token #" + std::to_string(i) + " changed from " + show(before, i) + " to " + show(after, i);
}

void compareTokens(const std::string& name, const std::string& original,
                   const std::string& formatted, CorpusReport& report) {
    LexResult before = lexAll(original);
    if (!before.ok) {
        ++report.skippedUnlexable;
        return;
    }
    ++report.tokenChecked;
    LexResult after = lexAll(formatted);
    if (!after.ok) {
        report.failures.push_back(name + ": formatted text no longer lexes: " + after.error);
        return;
    }
    SplitTokens a = splitLeadingImports(before.tokens);
    SplitTokens b = splitLeadingImports(after.tokens);
    if (a.imports != b.imports) {
        report.failures.push_back(name + ": the set of import statements changed");
    } else if (a.rest != b.rest) {
        report.failures.push_back(name + ": " + describeFirstDifference(a.rest, b.rest));
    }
}

void checkFile(const fs::path& file, const fs::path& root, CorpusReport& report) {
    std::error_code ec;
    fs::path relative = fs::relative(file, root, ec);
    const std::string name = ec ? file.string() : relative.generic_string();

    std::string original;
    try {
        lexer::FileReader diskReader;  // strips a UTF-8 BOM, keeps CRLF line endings
        original = diskReader.readFile(file.string());
    } catch (const std::exception& e) {
        report.failures.push_back(name + ": cannot read: " + e.what());
        return;
    }

    const FormattingOptions options{};  // what editors send: trim + final newline
    const std::string once = formatting::formatViaEdits(original, options);
    if (formatting::formatViaEdits(once, options) != once) {
        report.failures.push_back(name + ": formatting is not idempotent");
    }
    compareTokens(name, original, once, report);
}

std::optional<fs::path> findRepoRoot() {
    std::error_code ec;
    fs::path dir = fs::current_path(ec);
    while (!ec && !dir.empty()) {
        if (fs::exists(dir / "mType" / "tests" / "testFiles" / "lib", ec)) return dir;
        fs::path parent = dir.parent_path();
        if (parent == dir) break;
        dir = parent;
    }
    return std::nullopt;
}

std::vector<fs::path> collectCorpusFiles(const fs::path& testFiles) {
    std::vector<fs::path> files;
    for (const char* directory : kCorpusDirectories) {
        std::error_code ec;
        fs::recursive_directory_iterator it(testFiles / directory, ec);
        for (; !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
            std::error_code typeError;
            if (it->is_directory(typeError)) {
                if (it->path().filename() == "error") it.disable_recursion_pending();
            } else if (it->path().extension() == ".mt") {
                files.push_back(it->path());
            }
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

std::string summarize(const CorpusReport& report, std::size_t fileCount) {
    std::string message = std::to_string(report.failures.size()) + " problem(s) across "
        + std::to_string(fileCount) + " files (" + std::to_string(report.tokenChecked)
        + " token-checked, " + std::to_string(report.skippedUnlexable)
        + " skipped because the original does not lex)";
    for (std::size_t i = 0; i < report.failures.size() && i < kMaxReportedFailures; ++i) {
        message += "\n         - " + report.failures[i];
    }
    if (report.failures.size() > kMaxReportedFailures) message += "\n         - ...";
    return message;
}

} // namespace

void FormattingCorpusTestSuite::registerTests(LspTestHarness& harness) {
    harness.addTest("corpus: idempotent and token-preserving on real sources", []() {
        std::optional<fs::path> root = findRepoRoot();
        require(root.has_value(),
            "cannot find mType/tests/testFiles/lib above the working directory");

        std::vector<fs::path> files = collectCorpusFiles(*root / "mType" / "tests" / "testFiles");
        require(files.size() >= kMinimumCorpusFiles,
            "expected at least " + std::to_string(kMinimumCorpusFiles)
            + " corpus files, found " + std::to_string(files.size()));

        CorpusReport report;
        for (const auto& file : files) {
            checkFile(file, *root, report);
        }
        require(report.failures.empty(), summarize(report, files.size()));
    });
}

} // namespace mtype::lsp::test
