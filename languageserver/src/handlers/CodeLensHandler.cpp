#include "CodeLensHandler.hpp"
#include <regex>
#include <sstream>
#include <iostream>

namespace mtype::lsp {

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

CodeLensHandler::CodeLensHandler(DocumentManager* docMgr)
    : documentManager_(docMgr) {}

std::vector<CodeLens> CodeLensHandler::handleCodeLens(const std::string& uri) {
    std::vector<CodeLens> lenses;

    auto doc = documentManager_->getDocument(uri);
    if (!doc || !doc->environment) {
        return lenses;
    }

    auto classRegistry = doc->environment->getClassRegistry();
    auto interfaceRegistry = doc->environment->getInterfaceRegistry();
    if (!classRegistry && !interfaceRegistry) {
        return lenses;
    }

    // For each registered symbol, create a code lens with reference count
    for (const auto& [symbolName, location] : doc->symbolLocations) {
        // Check if it's a class or interface
        bool isClass = classRegistry && classRegistry->hasClass(symbolName);
        bool isInterface = interfaceRegistry && interfaceRegistry->hasInterface(symbolName);

        if (!isClass && !isInterface) {
            continue; // Skip non-class/interface symbols
        }

        // Count references to this symbol
        int refCount = countReferences(symbolName, uri);

        // Create code lens
        CodeLens lens;
        // Place range at start of line (character 0) so it renders above the declaration
        lens.range.start.line = location.line;
        lens.range.start.character = 0;
        lens.range.end.line = location.line;
        lens.range.end.character = 0;

        std::cerr << "[CodeLensHandler] Creating code lens for symbol: '" << symbolName << "'" << std::endl;
        std::cerr << "[CodeLensHandler] Reference count: " << refCount << std::endl;

        // Create command to show references using custom mtype.showReferences command
        Command cmd;
        if (refCount == 1) {
            cmd.title = "1 reference";
        } else {
            cmd.title = std::to_string(refCount) + " references";
        }
        cmd.command = "mtype.showReferences"; // Use custom command that converts JSON to VS Code types

        // Find actual reference locations
        std::vector<Location> refLocations = findUsages(symbolName, doc);
        std::cerr << "[CodeLensHandler] Found " << refLocations.size() << " reference locations" << std::endl;

        // Decode URI for VS Code command
        std::string decodedUri = urlDecode(uri);

        // Build arguments: uri, position, locations
        json args = json::array();
        args.push_back(decodedUri);

        json positionJson = {
            {"line", location.line},
            {"character", location.column}
        };
        args.push_back(positionJson);

        json locationsArray = json::array();
        for (const auto& loc : refLocations) {
            locationsArray.push_back(loc.toJson());
        }
        args.push_back(locationsArray);

        cmd.arguments = args;
        std::cerr << "[CodeLensHandler] Command: " << cmd.command << ", Args: " << args.dump() << std::endl;

        lens.command = cmd;
        lenses.push_back(lens);
    }

    return lenses;
}

int CodeLensHandler::countReferences(const std::string& symbolName, const std::string& uri) {
    int count = 0;

    auto doc = documentManager_->getDocument(uri);
    if (!doc) {
        return count;
    }

    const std::string& content = doc->content;

    // Patterns to look for:
    // 1. "new ClassName" or "new ClassName<"
    // 2. "implements InterfaceName" or "extends ClassName"
    // 3. ": ClassName" (type annotations)
    // 4. Variable declarations like "ClassName var"

    // Use regex to find references
    // Word boundary to ensure we match whole words only
    std::string pattern = "\\b" + symbolName + "\\b";
    std::regex symbolRegex(pattern);

    std::string line;
    std::istringstream stream(content);
    int lineNum = 0;

    while (std::getline(stream, line)) {
        // Count matches in this line
        auto words_begin = std::sregex_iterator(line.begin(), line.end(), symbolRegex);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            size_t matchPos = match.position();

            // Skip the declaration line itself (where the class/interface is defined)
            if (auto it = doc->symbolLocations.find(symbolName); it != doc->symbolLocations.end()) {
                if (lineNum == it->second.line) {
                    // This is the declaration, skip this match
                    continue;
                }
            }

            count++;
        }

        lineNum++;
    }

    return count;
}

std::vector<Location> CodeLensHandler::findUsages(const std::string& symbolName, const Document* doc) {
    std::vector<Location> locations;

    if (!doc) {
        return locations;
    }

    const std::string& content = doc->content;

    // Use regex to find all references to the symbol
    std::string pattern = "\\b" + symbolName + "\\b";
    std::regex symbolRegex(pattern);

    std::string line;
    std::istringstream stream(content);
    int lineNum = 0;

    while (std::getline(stream, line)) {
        // Find all matches in this line
        auto words_begin = std::sregex_iterator(line.begin(), line.end(), symbolRegex);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            size_t matchPos = match.position();

            // Skip the declaration line itself
            if (auto it = doc->symbolLocations.find(symbolName); it != doc->symbolLocations.end()) {
                if (lineNum == it->second.line) {
                    // This is the declaration, skip this match
                    continue;
                }
            }

            // Create a location for this reference
            Location loc;
            loc.uri = urlDecode(doc->uri); // Decode URI for VS Code
            loc.range.start.line = lineNum;
            loc.range.start.character = static_cast<int>(matchPos);
            loc.range.end.line = lineNum;
            loc.range.end.character = static_cast<int>(matchPos + symbolName.length());

            locations.push_back(loc);
        }

        lineNum++;
    }

    return locations;
}

} // namespace mtype::lsp
