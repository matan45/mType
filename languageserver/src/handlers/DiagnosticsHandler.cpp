#include "DiagnosticsHandler.hpp"
#include "../utils/UriUtils.hpp"
#include "../utils/LspDiagnosticConverter.hpp"

#include "../../../mType/diagnostics/DiagnosticBuilder.hpp"
#include "../../../mType/diagnostics/ErrorCodeRegistry.hpp"
#include "../../../mType/errors/SourceLocation.hpp"

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
        std::vector<Diagnostic> result;

        if (!doc) return result;

        // MYT-35 — Convert each shared core Diagnostic into the LSP wire
        // type via LspDiagnosticConverter. No regex on exception strings;
        // the structured spans, codes, notes, and secondary spans flow
        // directly through. The doc->diagnostics list is populated by
        // DocumentManager::parseDocument from caught exceptions.
        for (const auto& core : doc->diagnostics)
        {
            result.push_back(toLspDiagnostic(core, doc->uri));
        }

        // Import path validation produces core diagnostics too, then we
        // convert through the same path so the rendering is uniform.
        auto importDiagnostics = validateImportPaths(doc);
        result.insert(result.end(),
                      importDiagnostics.begin(),
                      importDiagnostics.end());

        return result;
    }

    std::vector<Diagnostic> DiagnosticsHandler::validateImportPaths(const Document* doc)
    {
        std::vector<Diagnostic> result;

        if (!doc) return result;

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
                    // Find the position of the path in the line. Source
                    // locations use 1-based line/column; the converter
                    // maps back to 0-based for LSP.
                    size_t pathStart = line.find('"') + 1;
                    size_t pathEnd = line.find('"', pathStart);

                    errors::SourceLocation start(
                        doc->uri, lineNumber + 1, static_cast<int>(pathStart) + 1);
                    errors::SourceLocation end(
                        doc->uri, lineNumber + 1, static_cast<int>(pathEnd) + 1);

                    auto coreDiag =
                        diagnostics::DiagnosticBuilder(diagnostics::codes::FileError)
                            .withMessage("cannot find module '" + importPath + "'")
                            .withPrimaryRange(start, end, "module not found")
                            .withSourceException("ImportResolver")
                            .build();
                    result.push_back(toLspDiagnostic(coreDiag, doc->uri));
                }
            }

            lineNumber++;
        }

        return result;
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
