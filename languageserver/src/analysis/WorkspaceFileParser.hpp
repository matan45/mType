#pragma once

#include "TypeGraphFacts.hpp"
#include "WorkspaceSymbol.hpp"

#include <optional>
#include <string>
#include <vector>

namespace mtype::lsp::analysis
{
    /** Everything the workspace index keeps for one file, from one parse. */
    struct ParsedWorkspaceFile
    {
        std::vector<WorkspaceSymbol> symbols; // top-level declarations (Ctrl+T, Add import)
        std::vector<TypeFacts> types;         // classes/interfaces with edges + members (MYT-432)
    };

    /**
     * Parses one workspace file for WorkspaceSymbolIndex. The file is parsed
     * once, and that one parse feeds both the symbol tables and the type graph.
     *
     * - Symbols come from the raw AST, so the companion classes that
     *   Lombok's `@Builder` generates stay out of Ctrl+T and Add import,
     *   as before.
     * - Type facts come from the AST after LombokSynthesisPass, the same
     *   pass DocumentManager runs, so closed files expose the same members
     *   as open ones.
     *
     * Returns nullopt when the content doesn't parse. The index then keeps
     * the file's last good entries.
     */
    class WorkspaceFileParser
    {
    public:
        static std::optional<ParsedWorkspaceFile> parse(const std::string& fileUri,
                                                        const std::string& content);
    };
}
