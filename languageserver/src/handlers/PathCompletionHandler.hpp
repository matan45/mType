#pragma once

#include <memory>
#include <vector>
#include <string>
#include "../utils/LSPTypes.hpp"
#include "../utils/ProjectConfigProvider.hpp"
#include "../DocumentManager.hpp"

namespace mtype::lsp {

class PathCompletionHandler {
public:
    explicit PathCompletionHandler(DocumentManager* docMgr);

    // MYT-309 — inject project config so @pkg/... import autocomplete can
    // enumerate aliases (both <Alias> entries and mt_modules/ packages).
    void setProjectConfig(std::shared_ptr<ProjectConfigProvider> config) { projectConfig_ = std::move(config); }

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

    // Emit one completion item per registered alias when the user's typed
    // prefix starts with `@` and has not yet been disambiguated by `/`.
    std::vector<CompletionItem> listAliasCompletions() const;

    // Check if a path is inside an import statement
    bool isInsideImportString(const std::string& content, const Position& position);

    DocumentManager* documentManager_;
    std::shared_ptr<ProjectConfigProvider> projectConfig_;
};

} // namespace mtype::lsp
