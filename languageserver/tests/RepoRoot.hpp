#pragma once

#include <filesystem>
#include <optional>
#include <system_error>

namespace mtype::lsp::test {

// The mType repository root: the first directory at or above the working
// directory that contains mType/tests/testFiles/lib. Corpus-style suites
// (formatting, unresolved-type sweep) read real sources from there, so the
// test binary must be run from inside the repository.
inline std::optional<std::filesystem::path> findRepoRoot() {
    namespace fs = std::filesystem;
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

} // namespace mtype::lsp::test
