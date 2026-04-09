#include "DiagnosticsHandler.hpp"
#include "../utils/UriUtils.hpp"
#include <sstream>
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

namespace mtype::lsp
{
    DiagnosticsHandler::DiagnosticsHandler(DocumentManager* docMgr)
        : documentManager_(docMgr)
    {
    }

    void DiagnosticsHandler::setPublisher(DiagnosticPublisher publisher)
    {
        publisher_ = std::move(publisher);
    }

    void DiagnosticsHandler::setProjectConfig(std::shared_ptr<ProjectConfigProvider> config)
    {
        projectConfig_ = std::move(config);
    }

    void DiagnosticsHandler::publishDiagnostics(const std::string& uri)
    {
        auto doc = documentManager_->getDocument(uri);
        if (!doc) return;

        auto diagnostics = analyzeDiagnostics(doc);

        if (publisher_)
        {
            publisher_(uri, diagnostics);
        }
    }

    std::vector<Diagnostic> DiagnosticsHandler::analyzeDiagnostics(const Document* doc)
    {
        std::vector<Diagnostic> diagnostics;

        if (!doc) return diagnostics;

        // Add parse errors
        for (const auto& error : doc->parseErrors)
        {
            Diagnostic diag;
            diag.severity = static_cast<int>(DiagnosticSeverity::Error);
            diag.message = "Parse error: " + error;
            diag.source = "mType Parser";

            // Try to extract line and column from error message
            // Format: "Error at file:///.../file.mt:LINE:COL: message"
            std::regex locationRegex(":([0-9]+):([0-9]+):");
            std::smatch match;
            if (std::regex_search(error, match, locationRegex) && match.size() >= 3)
            {
                int line = std::stoi(match[1].str());
                int col = std::stoi(match[2].str());
                // Convert from 1-based (parser) to 0-based (LSP)
                diag.range.start.line = line - 1;
                diag.range.start.character = col - 1;
                diag.range.end.line = line - 1;
                diag.range.end.character = col; // Highlight at least one character
            }
            else
            {
                // Fallback to line 0 if we can't parse the location
                diag.range = Range{{0, 0}, {0, 0}};
            }

            diagnostics.push_back(diag);
        }

        // Add semantic errors
        for (const auto& error : doc->semanticErrors)
        {
            Diagnostic diag;
            diag.range = Range{{0, 0}, {0, 0}}; // Default range
            diag.severity = static_cast<int>(DiagnosticSeverity::Error);
            diag.message = "Semantic error: " + error;
            diag.source = "mType Analyzer";
            diagnostics.push_back(diag);
        }

        // Add import path validation
        auto pathDiagnostics = validateImportPaths(doc);
        diagnostics.insert(diagnostics.end(), pathDiagnostics.begin(), pathDiagnostics.end());

        return diagnostics;
    }

    std::vector<Diagnostic> DiagnosticsHandler::validateImportPaths(const Document* doc)
    {
        std::vector<Diagnostic> diagnostics;

        if (!doc) return diagnostics;

        // Parse imports from content
        std::istringstream stream(doc->content);
        std::string line;
        int lineNumber = 0;

        // Regex to match: import ... from "path"
        std::regex importRegex("import\\s+.*\\s+from\\s+\"([^\"]+)\"");

        // Get the directory of the current file
        std::string currentFilePath = UriUtils::uriToFilePath(doc->uri);
        std::string currentDir = fs::path(currentFilePath).parent_path().string();

        while (std::getline(stream, line))
        {
            std::smatch match;
            if (std::regex_search(line, match, importRegex))
            {
                std::string importPath = match[1].str();

                bool exists = false;

                // First try: resolve using project config (search paths + aliases)
                if (projectConfig_ && projectConfig_->isLoaded())
                {
                    std::string resolved = projectConfig_->resolveImport(currentDir, importPath);
                    exists = !resolved.empty();
                }

                // Fallback: resolve relative to current file directory
                if (!exists)
                {
                    std::string resolvedPath = resolveImportPath(doc->uri, importPath);
                    exists = fs::exists(resolvedPath);
                }

                if (!exists)
                {
                    Diagnostic diag;

                    // Find the position of the path in the line
                    size_t pathStart = line.find('"') + 1;
                    size_t pathEnd = line.find('"', pathStart);

                    diag.range.start.line = lineNumber;
                    diag.range.start.character = static_cast<int>(pathStart);
                    diag.range.end.line = lineNumber;
                    diag.range.end.character = static_cast<int>(pathEnd);

                    diag.severity = static_cast<int>(DiagnosticSeverity::Error);
                    diag.message = "Cannot find module: '" + importPath + "'";
                    diag.source = "mType Import Resolver";

                    diagnostics.push_back(diag);
                }
            }

            lineNumber++;
        }

        return diagnostics;
    }

    std::string DiagnosticsHandler::resolveImportPath(const std::string& baseUri, const std::string& relativePath)
    {
        std::string basePath = UriUtils::uriToFilePath(baseUri);

        try
        {
            fs::path currentFilePath(basePath);
            fs::path currentDir = currentFilePath.parent_path();
            fs::path targetPath = currentDir / relativePath;
            return targetPath.lexically_normal().string();
        }
        catch (...)
        {
            return "";
        }
    }
} // namespace mtype::lsp
