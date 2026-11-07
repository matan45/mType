#pragma once

#include <string>

namespace mtype::lsp::analysis
{
    /**
     * A single top-level symbol discovered in a workspace `.mt` file.
     * Mirrors `SymbolLocationInfo` but adds a kind tag and stores the
     * file URI in fully-qualified form so the missing-import quick fix
     * can route across files.
     */
    enum class WorkspaceSymbolKind
    {
        Unknown,
        Class,
        Function,
        Interface
    };

    struct WorkspaceSymbol
    {
        std::string name;       // top-level only ("Foo", "bar"); members ("Foo.method") are filtered out
        std::string fileUri;    // file:// URI
        int line = 0;           // 0-based
        int column = 0;         // 0-based
        WorkspaceSymbolKind kind = WorkspaceSymbolKind::Unknown;
    };
}
