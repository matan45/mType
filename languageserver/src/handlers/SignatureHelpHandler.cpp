#include "SignatureHelpHandler.hpp"
#include <regex>
#include <sstream>

namespace mtype::lsp {

// --- SignatureHelp JSON serialization ---

json SignatureHelp::toJson() const {
    json result;

    json sigArray = json::array();
    for (const auto& sig : signatures) {
        // Build label: name(type param, type param): returnType
        std::string label = sig.name + "(";
        for (size_t i = 0; i < sig.parameters.size(); i++) {
            if (i > 0) label += ", ";
            label += sig.parameters[i].type + " " + sig.parameters[i].name;
        }
        label += "): " + sig.returnType;

        // Build documentation
        std::string doc = "**" + sig.name + "**\n\n";
        if (sig.isStatic) {
            doc += "_Static method_\n\n";
        }
        if (!sig.parameters.empty()) {
            doc += "**Parameters:**\n";
            for (const auto& param : sig.parameters) {
                doc += "- `" + param.name + "`: " + param.type + "\n";
            }
            doc += "\n";
        }
        doc += "**Returns:** " + sig.returnType;

        // Build parameter information
        json paramArray = json::array();
        for (const auto& param : sig.parameters) {
            std::string paramLabel = param.type + " " + param.name;
            paramArray.push_back(json{
                {"label", paramLabel},
                {"documentation", "Parameter: " + param.name + " (" + param.type + ")"}
            });
        }

        sigArray.push_back(json{
            {"label", label},
            {"documentation", json{{"kind", "markdown"}, {"value", doc}}},
            {"parameters", paramArray}
        });
    }

    result["signatures"] = sigArray;
    result["activeSignature"] = activeSignature;
    result["activeParameter"] = activeParameter;
    return result;
}

// --- SignatureHelpHandler ---

SignatureHelpHandler::SignatureHelpHandler(DocumentManager* docMgr)
    : documentManager_(docMgr)
{
}

std::optional<SignatureHelp> SignatureHelpHandler::handleSignatureHelp(
    const std::string& uri,
    const Position& position)
{
    auto* doc = documentManager_->getDocument(uri);
    if (!doc) return std::nullopt;

    // Extract line text up to cursor
    std::istringstream stream(doc->content);
    std::string line;
    int lineNum = 0;
    while (std::getline(stream, line)) {
        if (lineNum == position.line) break;
        lineNum++;
    }

    if (lineNum != position.line) return std::nullopt;

    std::string textBeforeCursor = line.substr(0, std::min(
        static_cast<size_t>(position.character), line.length()));

    auto callCtx = findFunctionCall(textBeforeCursor);
    if (!callCtx) return std::nullopt;

    std::vector<SignatureInfo> signatures;

    if (callCtx->isMethodCall && !callCtx->objectName.empty()) {
        signatures = findMethodSignatures(uri, callCtx->objectName,
                                          callCtx->functionName, position.line);
    } else {
        signatures = findFunctionSignatures(uri, callCtx->functionName, position.line);
    }

    // Only add built-in signature if no user-defined match was found,
    // to avoid showing both when the user shadows a built-in name.
    if (signatures.empty()) {
        auto builtin = getBuiltInSignature(callCtx->functionName);
        if (builtin) {
            signatures.push_back(*builtin);
        }
    }

    if (signatures.empty()) return std::nullopt;

    SignatureHelp help;
    help.signatures = std::move(signatures);
    help.activeSignature = 0;
    help.activeParameter = getActiveParameter(textBeforeCursor);
    return help;
}

std::optional<SignatureHelpHandler::FunctionCallContext>
SignatureHelpHandler::findFunctionCall(const std::string& textBeforeCursor) const {
    // Scan backwards for the outermost unmatched '(' to correctly
    // handle nested calls like obj.method(foo(), bar|.
    // The [^)]* approach breaks on nested parentheses.
    int depth = 0;
    int openParenPos = -1;
    bool inString = false;
    char stringChar = 0;

    for (int i = static_cast<int>(textBeforeCursor.length()) - 1; i >= 0; --i) {
        char c = textBeforeCursor[i];
        char prev = (i > 0) ? textBeforeCursor[i - 1] : 0;

        // Simple string tracking (backwards)
        if ((c == '"' || c == '\'') && prev != '\\') {
            if (inString && c == stringChar) {
                inString = false;
            } else if (!inString) {
                inString = true;
                stringChar = c;
            }
            continue;
        }
        if (inString) continue;

        if (c == ')') {
            depth++;
        } else if (c == '(') {
            if (depth > 0) {
                depth--;
            } else {
                openParenPos = i;
                break;
            }
        }
    }

    if (openParenPos < 0) return std::nullopt;

    // Extract the function/method name before the '('
    std::string beforeParen = textBeforeCursor.substr(0, openParenPos);
    // Trim trailing whitespace
    while (!beforeParen.empty() && std::isspace(static_cast<unsigned char>(beforeParen.back()))) {
        beforeParen.pop_back();
    }
    if (beforeParen.empty()) return std::nullopt;

    // Match method call: object.method
    std::regex methodRegex(R"((\w+)\.(\w+)$)");
    std::smatch methodMatch;
    if (std::regex_search(beforeParen, methodMatch, methodRegex)) {
        return FunctionCallContext{methodMatch[2].str(), true, methodMatch[1].str()};
    }

    // Match static method call: ClassName::method
    std::regex staticRegex(R"((\w+)::(\w+)$)");
    std::smatch staticMatch;
    if (std::regex_search(beforeParen, staticMatch, staticRegex)) {
        return FunctionCallContext{staticMatch[2].str(), true, staticMatch[1].str()};
    }

    // Match plain function call: functionName
    std::regex funcRegex(R"((\w+)$)");
    std::smatch funcMatch;
    if (std::regex_search(beforeParen, funcMatch, funcRegex)) {
        return FunctionCallContext{funcMatch[1].str(), false, ""};
    }

    return std::nullopt;
}

std::vector<SignatureInfo> SignatureHelpHandler::findMethodSignatures(
    const std::string& uri,
    const std::string& objectName,
    const std::string& methodName,
    int line) const
{
    std::vector<SignatureInfo> results;

    // Resolve the object's type
    std::string typeName = objectName;
    if (objectName != "this" && !std::isupper(static_cast<unsigned char>(objectName[0]))) {
        // Looks like a variable — try to resolve its type
        std::string resolvedType = documentManager_->getVariableType(uri, objectName, line);
        if (!resolvedType.empty()) {
            // Strip generics: List<int> -> List
            auto angleBracket = resolvedType.find('<');
            if (angleBracket != std::string::npos) {
                typeName = resolvedType.substr(0, angleBracket);
            } else {
                typeName = resolvedType;
            }
        }
    }

    // Search the document for method declarations in the target class
    auto* doc = documentManager_->getDocument(uri);
    if (!doc) return results;

    // Parse the document looking for the method declaration inside the class
    std::regex methodDeclRegex(
        R"(\b(?:public|private|protected)?\s*(?:static\s+)?(?:async\s+)?function\s+)" +
        methodName + R"(\s*\(([^)]*)\)\s*:\s*(\w+))");

    std::istringstream stream(doc->content);
    std::string currentLine;
    bool inTargetClass = false;
    int braceDepth = 0;

    while (std::getline(stream, currentLine)) {
        // Track if we're inside the target class
        if (currentLine.find("class " + typeName) != std::string::npos) {
            inTargetClass = true;
        }

        if (inTargetClass) {
            for (char c : currentLine) {
                if (c == '{') braceDepth++;
                else if (c == '}') braceDepth--;
            }
            if (braceDepth <= 0 && inTargetClass) {
                inTargetClass = false;
                braceDepth = 0;
            }
        }

        if (!inTargetClass) continue;

        std::smatch match;
        if (std::regex_search(currentLine, match, methodDeclRegex)) {
            SignatureInfo sig;
            sig.name = methodName;
            sig.returnType = match[2].str();
            sig.isStatic = currentLine.find("static") != std::string::npos;

            // Parse parameters
            std::string paramsStr = match[1].str();
            if (!paramsStr.empty()) {
                std::istringstream paramStream(paramsStr);
                std::string paramToken;
                while (std::getline(paramStream, paramToken, ',')) {
                    // Trim whitespace
                    size_t start = paramToken.find_first_not_of(" \t");
                    size_t end = paramToken.find_last_not_of(" \t");
                    if (start == std::string::npos) continue;
                    paramToken = paramToken.substr(start, end - start + 1);

                    // Parse "type name" pattern
                    std::regex paramRegex(R"((\w+(?:<[^>]+>)?)\s+(\w+))");
                    std::smatch paramMatch;
                    if (std::regex_search(paramToken, paramMatch, paramRegex)) {
                        ParameterInfo param;
                        param.type = paramMatch[1].str();
                        param.name = paramMatch[2].str();
                        sig.parameters.push_back(param);
                    }
                }
            }

            results.push_back(sig);
        }
    }

    return results;
}

std::vector<SignatureInfo> SignatureHelpHandler::findFunctionSignatures(
    const std::string& uri,
    const std::string& functionName,
    int line) const
{
    std::vector<SignatureInfo> results;
    auto* doc = documentManager_->getDocument(uri);
    if (!doc) return results;

    // Check if it's a constructor call (class name with uppercase first letter)
    if (!functionName.empty() && std::isupper(static_cast<unsigned char>(functionName[0]))) {
        // Look for constructor in the class
        std::regex constructorRegex(
            R"(\bfunction\s+constructor\s*\(([^)]*)\))");

        std::istringstream stream(doc->content);
        std::string currentLine;
        bool inTargetClass = false;
        int braceDepth = 0;

        while (std::getline(stream, currentLine)) {
            if (currentLine.find("class " + functionName) != std::string::npos) {
                inTargetClass = true;
            }

            if (inTargetClass) {
                for (char c : currentLine) {
                    if (c == '{') braceDepth++;
                    else if (c == '}') braceDepth--;
                }
                if (braceDepth <= 0 && inTargetClass) {
                    inTargetClass = false;
                    braceDepth = 0;
                }
            }

            if (!inTargetClass) continue;

            std::smatch match;
            if (std::regex_search(currentLine, match, constructorRegex)) {
                SignatureInfo sig;
                sig.name = functionName;
                sig.returnType = functionName;

                std::string paramsStr = match[1].str();
                if (!paramsStr.empty()) {
                    std::istringstream paramStream(paramsStr);
                    std::string paramToken;
                    while (std::getline(paramStream, paramToken, ',')) {
                        size_t start = paramToken.find_first_not_of(" \t");
                        size_t end = paramToken.find_last_not_of(" \t");
                        if (start == std::string::npos) continue;
                        paramToken = paramToken.substr(start, end - start + 1);

                        std::regex paramRegex(R"((\w+(?:<[^>]+>)?)\s+(\w+))");
                        std::smatch paramMatch;
                        if (std::regex_search(paramToken, paramMatch, paramRegex)) {
                            ParameterInfo param;
                            param.type = paramMatch[1].str();
                            param.name = paramMatch[2].str();
                            sig.parameters.push_back(param);
                        }
                    }
                }

                results.push_back(sig);
            }
        }
    }

    return results;
}

std::optional<SignatureInfo> SignatureHelpHandler::getBuiltInSignature(const std::string& name) const {
    if (name == "print") {
        return SignatureInfo{"print", {{"value", "any"}}, "void", false};
    }
    if (name == "hashCode") {
        return SignatureInfo{"hashCode", {{"object", "any"}}, "int", false};
    }
    if (name == "strLength") {
        return SignatureInfo{"strLength", {{"string", "string"}}, "int", false};
    }
    if (name == "parsePrimitive") {
        return SignatureInfo{"parsePrimitive", {{"value", "any"}}, "string", false};
    }
    return std::nullopt;
}

int SignatureHelpHandler::getActiveParameter(const std::string& textBeforeCursor) const {
    // Find the outermost unmatched '(' by scanning backwards,
    // matching the same logic as findFunctionCall.
    int depth = 0;
    int openParenPos = -1;
    bool inStr = false;
    char strCh = 0;

    for (int i = static_cast<int>(textBeforeCursor.length()) - 1; i >= 0; --i) {
        char c = textBeforeCursor[i];
        char prev = (i > 0) ? textBeforeCursor[i - 1] : 0;

        if ((c == '"' || c == '\'') && prev != '\\') {
            if (inStr && c == strCh) inStr = false;
            else if (!inStr) { inStr = true; strCh = c; }
            continue;
        }
        if (inStr) continue;

        if (c == ')') depth++;
        else if (c == '(') {
            if (depth > 0) depth--;
            else { openParenPos = i; break; }
        }
    }

    if (openParenPos < 0) return 0;

    std::string afterParen = textBeforeCursor.substr(openParenPos + 1);

    int paramIndex = 0;
    int nestDepth = 0;
    bool inString = false;
    char stringChar = 0;
    bool inInterpolation = false;
    int interpBraceDepth = 0;

    for (size_t i = 0; i < afterParen.length(); i++) {
        char c = afterParen[i];
        char prev = (i > 0) ? afterParen[i - 1] : 0;

        // Handle interpolated string start: $"
        if (!inString && c == '$' && i + 1 < afterParen.length() && afterParen[i + 1] == '"') {
            inString = true;
            inInterpolation = true;
            interpBraceDepth = 0;
            stringChar = '"';
            i++; // skip the "
            continue;
        }

        // Handle string literals
        if ((c == '"' || c == '\'') && prev != '\\') {
            if (!inString) {
                inString = true;
                stringChar = c;
            } else if (c == stringChar && (!inInterpolation || interpBraceDepth == 0)) {
                inString = false;
                inInterpolation = false;
            }
            continue;
        }

        if (inString) {
            if (inInterpolation) {
                if (c == '{' && prev != '\\') interpBraceDepth++;
                else if (c == '}' && prev != '\\') interpBraceDepth--;
            }
            continue;
        }

        // Handle nesting
        if (c == '(' || c == '[' || c == '<') nestDepth++;
        else if (c == ')' || c == ']' || c == '>') nestDepth--;
        else if (c == ',' && nestDepth == 0) paramIndex++;
    }

    return paramIndex;
}

} // namespace mtype::lsp
