#pragma once

#include <string>
#include <vector>
#include "../utils/LSPTypes.hpp"
#include "../DocumentManager.hpp"
#include "../analysis/WorkspaceSymbolIndex.hpp"

namespace mtype::lsp {

class ReferencesHandler {
public:
    ReferencesHandler(DocumentManager* docMgr,
                      std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex = nullptr);

    std::vector<Location> handleReferences(
        const std::string& uri,
        const Position& position,
        bool includeDeclaration
    );

private:
    // Determine what kind of symbol is at the cursor
    struct SymbolContext {
        std::string name;
        std::string type;       // "class", "method", "field", "variable", "constructor"
        std::string className;  // owning class (empty for top-level symbols)
    };

    SymbolContext getSymbolContext(
        const std::string& word,
        const std::string& line,
        const std::string& uri
    ) const;

    // Find references within a single document
    std::vector<Location> findReferencesInDocument(
        const std::string& symbolName,
        const std::string& uri,
        const std::string& content
    ) const;

    // Find class-specific references (type annotations, new, extends, implements, static access)
    std::vector<Location> findClassReferences(
        const std::string& className,
        const std::string& uri,
        const std::string& content,
        bool includeDeclaration
    ) const;

    // Find variable references (whole-word matches)
    std::vector<Location> findVariableReferences(
        const std::string& varName,
        const std::string& uri,
        const std::string& content,
        bool includeDeclaration
    ) const;

    // Check if a character position is a whole-word boundary
    static bool isWordBoundary(char c);

    DocumentManager* documentManager_;
    std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex_;
};

} // namespace mtype::lsp
