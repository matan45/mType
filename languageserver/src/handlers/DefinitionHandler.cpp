#include "DefinitionHandler.hpp"
#include <iostream>
#include <algorithm>

namespace mtype::lsp {

// Helper function to convert absolute file path to file:/// URI
static std::string pathToUri(const std::string& path) {
    // If it's already a URI (starts with file:///), return as-is
    if (path.find("file:///") == 0) {
        return path;
    }

    std::string result = path;

    // Convert backslashes to forward slashes
    std::replace(result.begin(), result.end(), '\\', '/');

    // Check if it's an absolute Windows path (e.g., C:/...)
    if (result.length() >= 2 && result[1] == ':') {
        // Convert drive letter to lowercase
        result[0] = std::tolower(result[0]);
        // Add file:/// prefix
        result = "file:///" + result;
    }
    // Check if it's already an absolute Unix path (starts with /)
    else if (result.length() >= 1 && result[0] == '/') {
        result = "file://" + result;
    }

    std::cerr << "[DefinitionHandler::pathToUri] Converted: " << path << " -> " << result << std::endl;
    return result;
}

std::optional<Location> DefinitionHandler::handleDefinition(const std::string& uri, const Position& position) {
    std::cerr << "[DefinitionHandler] handleDefinition called for uri: " << uri
              << " at line: " << position.line << ", character: " << position.character << std::endl;

    auto symbolLoc = documentManager_->findDefinition(uri, position.line, position.character);

    if (!symbolLoc) {
        std::cerr << "[DefinitionHandler] No definition found" << std::endl;
        return std::nullopt;
    }

    std::cerr << "[DefinitionHandler] Definition found at " << symbolLoc->uri
              << ":" << symbolLoc->line << ":" << symbolLoc->column << std::endl;

    Location location;
    // Convert absolute path to file:/// URI
    location.uri = pathToUri(symbolLoc->uri);
    location.range.start.line = symbolLoc->line;
    location.range.start.character = symbolLoc->column;
    location.range.end.line = symbolLoc->line;
    location.range.end.character = symbolLoc->column;

    return location;
}

} // namespace mtype::lsp
