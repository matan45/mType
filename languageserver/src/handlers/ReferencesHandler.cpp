#include "ReferencesHandler.hpp"
#include "../utils/UriUtils.hpp"
#include <fstream>
#include <regex>
#include <sstream>
#include <filesystem>

namespace mtype::lsp {

ReferencesHandler::ReferencesHandler(
    DocumentManager* docMgr,
    std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex)
    : documentManager_(docMgr)
    , workspaceIndex_(std::move(workspaceIndex))
{
}

std::vector<Location> ReferencesHandler::handleReferences(
    const std::string& uri,
    const Position& position,
    bool includeDeclaration)
{
    auto* doc = documentManager_->getDocument(uri);
    if (!doc) return {};

    std::string word = documentManager_->getWordAtPosition(uri, position.line, position.character);
    if (word.empty()) return {};

    const std::string& content = doc->content;
    std::istringstream stream(content);
    std::string currentLine;
    int lineNum = 0;
    while (std::getline(stream, currentLine)) {
        if (lineNum == position.line) break;
        lineNum++;
    }

    auto ctx = getSymbolContext(word, currentLine, uri);

    std::vector<Location> results;

    // Find references in the current document
    if (ctx.type == "class") {
        auto refs = findClassReferences(word, uri, content, includeDeclaration);
        results.insert(results.end(), refs.begin(), refs.end());
    } else {
        auto refs = findVariableReferences(word, uri, content, includeDeclaration);
        results.insert(results.end(), refs.begin(), refs.end());
    }

    // Find cross-file references via workspace index
    if (workspaceIndex_) {
        workspaceIndex_->waitForReady(std::chrono::milliseconds(200));
        auto allFiles = workspaceIndex_->getAllIndexedFiles();

        for (const auto& fileUri : allFiles) {
            if (fileUri == uri) continue;

            // Try to get content from the document manager (open files)
            auto* otherDoc = documentManager_->getDocument(fileUri);
            std::string otherContent;
            if (otherDoc) {
                otherContent = otherDoc->content;
            } else {
                // Read from disk
                std::string filePath = UriUtils::uriToFilePath(fileUri);
                std::ifstream file(filePath);
                if (!file.is_open()) continue;
                std::ostringstream ss;
                ss << file.rdbuf();
                otherContent = ss.str();
            }

            auto refs = findReferencesInDocument(word, fileUri, otherContent);
            results.insert(results.end(), refs.begin(), refs.end());
        }
    }

    return results;
}

ReferencesHandler::SymbolContext ReferencesHandler::getSymbolContext(
    const std::string& word,
    const std::string& line,
    const std::string& uri) const
{
    SymbolContext ctx;
    ctx.name = word;

    // Check for static access: ClassName::member
    std::regex staticRegex(R"((\w+)::\s*)" + word);
    std::smatch staticMatch;
    if (std::regex_search(line, staticMatch, staticRegex)) {
        ctx.className = staticMatch[1].str();
        ctx.type = "method"; // could be field, but doesn't matter for search
        return ctx;
    }

    // Check for instance access: object.member
    std::regex instanceRegex(R"((\w+)\.\s*)" + word);
    std::smatch instanceMatch;
    if (std::regex_search(line, instanceMatch, instanceRegex)) {
        std::string objectName = instanceMatch[1].str();
        std::string objectType = documentManager_->getVariableType(uri, objectName, 0);
        ctx.className = objectType;
        ctx.type = "method";
        return ctx;
    }

    // Check for constructor: new ClassName
    std::regex newRegex(R"(\bnew\s+)" + word + R"(\s*\()");
    if (std::regex_search(line, newRegex)) {
        ctx.type = "constructor";
        ctx.className = word;
        return ctx;
    }

    // Check if word is a known class via document symbols
    auto* doc = documentManager_->getDocument(uri);
    if (doc) {
        auto symbols = documentManager_->getDocumentSymbols(uri);
        for (const auto& sym : symbols) {
            if (sym.name == word && sym.kind == "class") {
                ctx.type = "class";
                return ctx;
            }
        }
    }

    // Default to variable
    ctx.type = "variable";
    return ctx;
}

std::vector<Location> ReferencesHandler::findReferencesInDocument(
    const std::string& symbolName,
    const std::string& uri,
    const std::string& content) const
{
    std::vector<Location> refs;
    std::istringstream stream(content);
    std::string line;
    int lineNum = 0;

    while (std::getline(stream, line)) {
        size_t startIdx = 0;
        while (true) {
            size_t pos = line.find(symbolName, startIdx);
            if (pos == std::string::npos) break;

            // Check word boundaries
            char prevChar = (pos > 0) ? line[pos - 1] : ' ';
            char nextChar = (pos + symbolName.length() < line.length())
                            ? line[pos + symbolName.length()] : ' ';

            if (isWordBoundary(prevChar) && isWordBoundary(nextChar)) {
                Location loc;
                loc.uri = uri;
                loc.range.start.line = lineNum;
                loc.range.start.character = static_cast<int>(pos);
                loc.range.end.line = lineNum;
                loc.range.end.character = static_cast<int>(pos + symbolName.length());
                refs.push_back(loc);
            }

            startIdx = pos + 1;
        }
        lineNum++;
    }

    return refs;
}

std::vector<Location> ReferencesHandler::findClassReferences(
    const std::string& className,
    const std::string& uri,
    const std::string& content,
    bool includeDeclaration) const
{
    std::vector<Location> refs;
    std::istringstream stream(content);
    std::string line;
    int lineNum = 0;

    while (std::getline(stream, line)) {
        // Skip declaration line if not included
        if (!includeDeclaration) {
            std::regex declRegex(R"(^\s*(?:abstract\s+)?(?:class|interface)\s+)" + className + R"(\b)");
            if (std::regex_search(line, declRegex)) {
                lineNum++;
                continue;
            }
        }

        // Find all whole-word occurrences of the class name
        size_t startIdx = 0;
        while (true) {
            size_t pos = line.find(className, startIdx);
            if (pos == std::string::npos) break;

            char prevChar = (pos > 0) ? line[pos - 1] : ' ';
            char nextChar = (pos + className.length() < line.length())
                            ? line[pos + className.length()] : ' ';

            if (isWordBoundary(prevChar) && isWordBoundary(nextChar)) {
                Location loc;
                loc.uri = uri;
                loc.range.start.line = lineNum;
                loc.range.start.character = static_cast<int>(pos);
                loc.range.end.line = lineNum;
                loc.range.end.character = static_cast<int>(pos + className.length());
                refs.push_back(loc);
            }

            startIdx = pos + 1;
        }
        lineNum++;
    }

    return refs;
}

std::vector<Location> ReferencesHandler::findVariableReferences(
    const std::string& varName,
    const std::string& uri,
    const std::string& content,
    bool includeDeclaration) const
{
    // For variables, we just do whole-word search across all lines
    // (same as findReferencesInDocument, but optionally skipping the declaration)
    std::vector<Location> refs;
    std::istringstream stream(content);
    std::string line;
    int lineNum = 0;
    bool foundDeclaration = false;

    while (std::getline(stream, line)) {
        size_t startIdx = 0;
        while (true) {
            size_t pos = line.find(varName, startIdx);
            if (pos == std::string::npos) break;

            char prevChar = (pos > 0) ? line[pos - 1] : ' ';
            char nextChar = (pos + varName.length() < line.length())
                            ? line[pos + varName.length()] : ' ';

            if (isWordBoundary(prevChar) && isWordBoundary(nextChar)) {
                // Skip the first occurrence if it looks like a declaration and includeDeclaration is false
                if (!includeDeclaration && !foundDeclaration) {
                    // Check if this line has a declaration pattern
                    std::regex declRegex(R"(\b\w+\s+)" + varName + R"(\s*[;=])");
                    if (std::regex_search(line, declRegex)) {
                        foundDeclaration = true;
                        startIdx = pos + 1;
                        continue;
                    }
                }

                Location loc;
                loc.uri = uri;
                loc.range.start.line = lineNum;
                loc.range.start.character = static_cast<int>(pos);
                loc.range.end.line = lineNum;
                loc.range.end.character = static_cast<int>(pos + varName.length());
                refs.push_back(loc);
            }

            startIdx = pos + 1;
        }
        lineNum++;
    }

    return refs;
}

bool ReferencesHandler::isWordBoundary(char c) {
    return !(std::isalnum(static_cast<unsigned char>(c)) || c == '_');
}

} // namespace mtype::lsp
