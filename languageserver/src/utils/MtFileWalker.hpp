#pragma once

#include <string>
#include <vector>

namespace mtype::lsp::utils
{
    /**
     * Recursively scan a workspace root for `.mt` source files.
     *
     * Skips hidden directories (names starting with `.`), `node_modules`,
     * and `bin`/`obj` build output. Returns a sorted vector of absolute
     * paths so the workspace symbol index can iterate deterministically.
     *
     * Failures (permission denied, broken symlinks, missing root) are
     * silently dropped — the LSP should not crash on a malformed workspace.
     */
    class MtFileWalker
    {
    public:
        static std::vector<std::string> findMtFiles(const std::string& workspaceRoot);
    };
}
