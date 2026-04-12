#include "MtFileWalkerTestSuite.hpp"
#include "../src/utils/MtFileWalker.hpp"
#include "TempDir.hpp"

#include <algorithm>

namespace mtype::lsp::test {

void MtFileWalkerTestSuite::registerTests(LspTestHarness& harness) {

    harness.addTest("finds .mt files in root directory", []() {
        TempDir tmpDir;
        tmpDir.createFile("foo.mt", "class Foo {}");
        tmpDir.createFile("bar.mt", "class Bar {}");
        tmpDir.createFile("readme.txt", "not a source file");

        auto files = utils::MtFileWalker::findMtFiles(tmpDir.path());

        require(files.size() >= 2, "expected at least 2 .mt files, got "
            + std::to_string(files.size()));

        bool foundFoo = false, foundBar = false;
        for (const auto& f : files) {
            if (f.find("foo.mt") != std::string::npos) foundFoo = true;
            if (f.find("bar.mt") != std::string::npos) foundBar = true;
        }
        require(foundFoo, "expected foo.mt");
        require(foundBar, "expected bar.mt");
    });

    harness.addTest("recursively finds .mt files in subdirectories", []() {
        TempDir tmpDir;
        tmpDir.createFile("root.mt", "class Root {}");
        tmpDir.createFile("sub/nested.mt", "class Nested {}");
        tmpDir.createFile("sub/deep/deep.mt", "class Deep {}");

        auto files = utils::MtFileWalker::findMtFiles(tmpDir.path());

        require(files.size() >= 3, "expected at least 3 .mt files, got "
            + std::to_string(files.size()));
    });

    harness.addTest("skips hidden directories", []() {
        TempDir tmpDir;
        tmpDir.createFile("visible.mt", "class Visible {}");
        tmpDir.createFile(".hidden/secret.mt", "class Secret {}");

        auto files = utils::MtFileWalker::findMtFiles(tmpDir.path());

        bool foundHidden = false;
        for (const auto& f : files) {
            if (f.find("secret.mt") != std::string::npos) foundHidden = true;
        }
        require(!foundHidden, "should skip files in hidden directories");
    });

    harness.addTest("skips node_modules", []() {
        TempDir tmpDir;
        tmpDir.createFile("src.mt", "class Src {}");
        tmpDir.createFile("node_modules/dep.mt", "class Dep {}");

        auto files = utils::MtFileWalker::findMtFiles(tmpDir.path());

        bool foundNodeModules = false;
        for (const auto& f : files) {
            if (f.find("node_modules") != std::string::npos) foundNodeModules = true;
        }
        require(!foundNodeModules, "should skip node_modules directory");
    });

    harness.addTest("skips bin and build directories", []() {
        TempDir tmpDir;
        tmpDir.createFile("src.mt", "class Src {}");
        tmpDir.createFile("bin/output.mt", "class Output {}");
        tmpDir.createFile("build/gen.mt", "class Gen {}");

        auto files = utils::MtFileWalker::findMtFiles(tmpDir.path());

        for (const auto& f : files) {
            require(f.find("/bin/") == std::string::npos && f.find("\\bin\\") == std::string::npos,
                "should skip bin directory");
            require(f.find("/build/") == std::string::npos && f.find("\\build\\") == std::string::npos,
                "should skip build directory");
        }
    });

    harness.addTest("results are sorted", []() {
        TempDir tmpDir;
        tmpDir.createFile("z.mt", "class Z {}");
        tmpDir.createFile("a.mt", "class A {}");
        tmpDir.createFile("m.mt", "class M {}");

        auto files = utils::MtFileWalker::findMtFiles(tmpDir.path());
        require(files.size() >= 3, "expected at least 3 files");

        // Normalize paths (backslash → forward slash) before comparing,
        // since std::sort on mixed separators is platform-dependent.
        auto normalize = [](const std::string& p) {
            std::string n = p;
            for (char& c : n) { if (c == '\\') c = '/'; }
            return n;
        };
        for (size_t i = 1; i < files.size(); ++i) {
            require(normalize(files[i - 1]) <= normalize(files[i]),
                "results should be sorted");
        }
    });

    harness.addTest("nonexistent root returns empty", []() {
        auto files = utils::MtFileWalker::findMtFiles("C:/nonexistent/path/12345");
        require(files.empty(), "expected empty for nonexistent root");
    });
}

} // namespace mtype::lsp::test
