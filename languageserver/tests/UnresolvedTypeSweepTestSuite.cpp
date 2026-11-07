#include "UnresolvedTypeSweepTestSuite.hpp"
#include "RepoRoot.hpp"
#include "../src/DocumentManager.hpp"
#include "../src/utils/UriUtils.hpp"
#include "../../mType/services/FileReader.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace mtype::lsp::test {

namespace {

namespace fs = std::filesystem;

constexpr std::size_t kMinimumLibFiles = 90; // 96 today; guards against an empty walk
constexpr std::size_t kMaxReportedFailures = 25;

struct SweepReport {
    std::vector<std::string> failures;
    std::size_t files = 0;
};

fs::path repoRoot() {
    auto root = findRepoRoot();
    require(root.has_value(), "cannot find mType/tests/testFiles/lib above the working directory");
    return *root;
}

std::vector<fs::path> mtFilesUnder(const fs::path& dir) {
    std::vector<fs::path> files;
    std::error_code ec;
    fs::recursive_directory_iterator it(dir, ec);
    for (; !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (it->path().extension() == ".mt") files.push_back(it->path());
    }
    std::sort(files.begin(), files.end());
    return files;
}

// "line:col Name" (1-based, as an editor shows it) for each MT-E1003.
std::vector<std::string> unresolvedIn(const Document& doc) {
    std::vector<std::string> found;
    for (const auto& d : doc.diagnostics) {
        if (!d.code || std::string(d.code->id) != "MT-E1003") continue;
        found.push_back(std::to_string(d.primarySpan.start.getLine()) + ":"
            + std::to_string(d.primarySpan.start.getColumn()) + " " + d.message);
    }
    return found;
}

std::string relativeName(const fs::path& file, const fs::path& root) {
    std::error_code ec;
    const fs::path rel = fs::relative(file, root, ec);
    return (ec ? file : rel).generic_string();
}

// Parses the file twice in one DocumentManager: the second parse goes
// through the import cache, which used to drop transitive imports.
void sweepFile(DocumentManager& docMgr, const fs::path& file, const fs::path& root,
               SweepReport& report) {
    lexer::FileReader reader; // strips a UTF-8 BOM, keeps CRLF line endings
    const std::string source = reader.readFile(file.string());
    const std::string uri = UriUtils::filePathToUri(file.string());
    docMgr.openDocument(uri, source, 1);
    docMgr.updateDocument(uri, source, 2);
    for (const auto& hit : unresolvedIn(*docMgr.getDocument(uri))) {
        report.failures.push_back(relativeName(file, root) + ":" + hit);
    }
    docMgr.closeDocument(uri);
    ++report.files;
}

// Positive control: a probe document that imports `file` and uses one
// unknown name must get exactly that one diagnostic. Zero means the
// analysis bailed (some import in the file's closure didn't resolve), and
// then the sweep above proved nothing for this file.
void probeFile(DocumentManager& docMgr, const fs::path& file, const fs::path& root,
               SweepReport& report) {
    const fs::path probePath = file.parent_path() / "__unresolved_type_probe__.mt";
    const std::string uri = UriUtils::filePathToUri(probePath.string());
    const std::string source = "import * from \"" + file.filename().generic_string()
        + "\";\nZzProbe zzProbe = null;\n";
    docMgr.openDocument(uri, source, 1);
    const auto found = unresolvedIn(*docMgr.getDocument(uri));
    docMgr.closeDocument(uri);
    ++report.files;
    if (found.size() != 1) {
        report.failures.push_back(relativeName(file, root) + ": probe got "
            + std::to_string(found.size()) + " diagnostic(s), expected 1");
    }
}

std::string summarize(const SweepReport& report) {
    std::string message = std::to_string(report.failures.size()) + " problem(s) across "
        + std::to_string(report.files) + " files";
    for (std::size_t i = 0; i < report.failures.size() && i < kMaxReportedFailures; ++i) {
        message += "\n         - " + report.failures[i];
    }
    if (report.failures.size() > kMaxReportedFailures) message += "\n         - ...";
    return message;
}

} // namespace

void UnresolvedTypeSweepTestSuite::registerTests(LspTestHarness& harness) {
    harness.addTest("stdlib and examples: zero unresolved-type diagnostics", []() {
        const fs::path root = repoRoot();
        const auto libFiles = mtFilesUnder(root / "mType" / "tests" / "testFiles" / "lib");
        require(libFiles.size() >= kMinimumLibFiles,
            "expected at least " + std::to_string(kMinimumLibFiles) + " stdlib files, found "
            + std::to_string(libFiles.size()));

        DocumentManager docMgr;
        SweepReport report;
        for (const auto& file : libFiles) sweepFile(docMgr, file, root, report);
        for (const auto& file : mtFilesUnder(root / "examples")) sweepFile(docMgr, file, root, report);
        require(report.failures.empty(), summarize(report));
    });

    harness.addTest("stdlib: every file's imports resolve, so the sweep checks it", []() {
        const fs::path root = repoRoot();
        DocumentManager docMgr;
        SweepReport report;
        for (const auto& file : mtFilesUnder(root / "mType" / "tests" / "testFiles" / "lib")) {
            probeFile(docMgr, file, root, report);
        }
        require(report.failures.empty(), summarize(report));
    });

    harness.addTest("examples carry no project config the sweep would ignore", []() {
        // The sweep runs without a ProjectConfigProvider. If an example gains
        // mt_modules aliases, its imports stop resolving here and the sweep
        // silently stops checking it, so make that loud.
        const fs::path root = repoRoot();
        std::error_code ec;
        for (fs::recursive_directory_iterator it(root / "examples", ec);
             !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
            const std::string name = it->path().filename().string();
            require(name != "mtpkg.json" && name != ".mtproj",
                "examples now has " + relativeName(it->path(), root)
                + "; load a ProjectConfigProvider for it in this sweep");
        }
    });
}

} // namespace mtype::lsp::test
