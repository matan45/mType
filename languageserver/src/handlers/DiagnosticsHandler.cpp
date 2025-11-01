#include "DiagnosticsHandler.hpp"
#include <sstream>
#include <regex>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace mtype::lsp {

DiagnosticsHandler::DiagnosticsHandler(DocumentManager* docMgr)
    : documentManager_(docMgr) {}

void DiagnosticsHandler::setPublisher(DiagnosticPublisher publisher) {
    publisher_ = std::move(publisher);
}

void DiagnosticsHandler::publishDiagnostics(const std::string& uri) {
    auto doc = documentManager_->getDocument(uri);
    if (!doc) return;

    auto diagnostics = analyzeDiagnostics(doc);

    if (publisher_) {
        publisher_(uri, diagnostics);
    }
}

std::vector<Diagnostic> DiagnosticsHandler::analyzeDiagnostics(const Document* doc) {
    std::vector<Diagnostic> diagnostics;

    if (!doc) return diagnostics;

    // Add parse errors
    for (const auto& error : doc->parseErrors) {
        Diagnostic diag;
        diag.severity = static_cast<int>(DiagnosticSeverity::Error);
        diag.message = "Parse error: " + error;
        diag.source = "mType Parser";

        // Try to extract line and column from error message
        // Format: "Error at file:///.../file.mt:LINE:COL: message"
        std::regex locationRegex(":([0-9]+):([0-9]+):");
        std::smatch match;
        if (std::regex_search(error, match, locationRegex) && match.size() >= 3) {
            int line = std::stoi(match[1].str());
            int col = std::stoi(match[2].str());
            std::cerr << "[DiagnosticsHandler] Parsed error location: line=" << line
                      << ", col=" << col << " from error: " << error << std::endl;
            // Convert from 1-based (parser) to 0-based (LSP)
            diag.range.start.line = line - 1;
            diag.range.start.character = col - 1;
            diag.range.end.line = line - 1;
            diag.range.end.character = col; // Highlight at least one character
            std::cerr << "[DiagnosticsHandler] LSP range: (" << diag.range.start.line
                      << "," << diag.range.start.character << ") to ("
                      << diag.range.end.line << "," << diag.range.end.character << ")" << std::endl;
        } else {
            std::cerr << "[DiagnosticsHandler] Could not parse location from error: " << error << std::endl;
            // Fallback to line 0 if we can't parse the location
            diag.range = Range{{0, 0}, {0, 0}};
        }

        diagnostics.push_back(diag);
    }

    // Add semantic errors
    for (const auto& error : doc->semanticErrors) {
        Diagnostic diag;
        diag.range = Range{{0, 0}, {0, 0}};  // Default range
        diag.severity = static_cast<int>(DiagnosticSeverity::Error);
        diag.message = "Semantic error: " + error;
        diag.source = "mType Analyzer";
        diagnostics.push_back(diag);
    }

    // Add import path validation
    auto pathDiagnostics = validateImportPaths(doc);
    diagnostics.insert(diagnostics.end(), pathDiagnostics.begin(), pathDiagnostics.end());

    // TODO: Add more sophisticated error checking with proper source locations
    // - Type errors with specific line/column
    // - Undefined variables
    // - Unused variables (warning)
    // - etc.

    return diagnostics;
}

std::vector<Diagnostic> DiagnosticsHandler::validateImportPaths(const Document* doc) {
    std::vector<Diagnostic> diagnostics;

    if (!doc) return diagnostics;

    // Parse imports from content
    std::istringstream stream(doc->content);
    std::string line;
    int lineNumber = 0;

    // Regex to match: import ... from "path"
    std::regex importRegex("import\\s+.*\\s+from\\s+\"([^\"]+)\"");

    std::cerr << "[DiagnosticsHandler] Validating imports for: " << doc->uri << std::endl;

    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, importRegex)) {
            std::string importPath = match[1].str();
            std::cerr << "[DiagnosticsHandler] Found import: '" << importPath << "' on line " << lineNumber << std::endl;

            // Resolve relative path
            std::string resolvedPath = resolveImportPath(doc->uri, importPath);
            std::cerr << "[DiagnosticsHandler] Resolved to: '" << resolvedPath << "'" << std::endl;

            // Check if path exists
            bool exists = fs::exists(resolvedPath);
            std::cerr << "[DiagnosticsHandler] Path exists: " << (exists ? "YES" : "NO") << std::endl;

            if (!exists) {
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

                std::cerr << "[DiagnosticsHandler] Adding diagnostic for missing import" << std::endl;
                diagnostics.push_back(diag);
            }
        }

        lineNumber++;
    }

    std::cerr << "[DiagnosticsHandler] Total diagnostics: " << diagnostics.size() << std::endl;
    return diagnostics;
}

// Helper function to URL decode a string
static std::string urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            // Convert hex to char
            int value;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }

    return result;
}

std::string DiagnosticsHandler::resolveImportPath(const std::string& baseUri, const std::string& relativePath) {
    std::cerr << "[resolveImportPath] Input baseUri: '" << baseUri << "'" << std::endl;
    std::cerr << "[resolveImportPath] Input relativePath: '" << relativePath << "'" << std::endl;

    // Convert file:// URI to filesystem path
    std::string basePath = baseUri;

    // Remove file:/// prefix if present
    const std::string filePrefix = "file:///";
    if (basePath.find(filePrefix) == 0) {
        basePath = basePath.substr(filePrefix.length());
        std::cerr << "[resolveImportPath] After removing file:/// prefix: '" << basePath << "'" << std::endl;

        // URL decode the path (e.g., %3A -> :)
        basePath = urlDecode(basePath);
        std::cerr << "[resolveImportPath] After URL decode: '" << basePath << "'" << std::endl;

        // On Windows, convert /C:/path to C:/path
        if (basePath.length() >= 3 && basePath[0] == '/' && basePath[2] == ':') {
            basePath = basePath.substr(1);
            std::cerr << "[resolveImportPath] After Windows path fix: '" << basePath << "'" << std::endl;
        }
    }

    try {
        // Get the directory containing the current file
        fs::path currentFilePath(basePath);
        fs::path currentDir = currentFilePath.parent_path();
        std::cerr << "[resolveImportPath] Current directory: '" << currentDir.string() << "'" << std::endl;

        // Resolve the relative path
        fs::path targetPath = currentDir / relativePath;
        std::cerr << "[resolveImportPath] Target path (before normalize): '" << targetPath.string() << "'" << std::endl;

        // Normalize the path
        std::string result = targetPath.lexically_normal().string();
        std::cerr << "[resolveImportPath] Final resolved path: '" << result << "'" << std::endl;
        return result;
    } catch (const std::exception& e) {
        std::cerr << "[resolveImportPath] Exception: " << e.what() << std::endl;
        return "";
    } catch (...) {
        std::cerr << "[resolveImportPath] Unknown exception" << std::endl;
        return "";
    }
}

} // namespace mtype::lsp
