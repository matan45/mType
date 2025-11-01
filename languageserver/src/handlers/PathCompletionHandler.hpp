#pragma once

#include <vector>
#include <string>
#include "../utils/LSPTypes.hpp"
#include "../DocumentManager.hpp"

namespace mtype::lsp {

class PathCompletionHandler {
public:
    explicit PathCompletionHandler(DocumentManager* docMgr);

    // Get path completions for import statements
    std::vector<CompletionItem> getPathCompletions(
        const std::string& uri,
        const Position& position
    );

private:
    // Extract the current path being typed in an import statement
    std::string extractImportPath(const std::string& content, const Position& position);

    // Get directory path relative to the current file
    std::string resolveRelativePath(const std::string& baseUri, const std::string& relativePath);

    // List files and folders in a directory
    std::vector<CompletionItem> listDirectoryContents(
        const std::string& directoryPath,
        bool includeFiles = true,
        bool includeFolders = true
    );

    // Check if a path is inside an import statement
    bool isInsideImportString(const std::string& content, const Position& position);

    DocumentManager* documentManager_;
};

} // namespace mtype::lsp
