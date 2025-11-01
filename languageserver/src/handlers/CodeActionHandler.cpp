#include "CodeActionHandler.hpp"
#include "../../../mType/runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../../mType/runtimeTypes/klass/ClassDefinition.hpp"
#include <sstream>

namespace mtype::lsp {

CodeActionHandler::CodeActionHandler(DocumentManager* docMgr)
    : documentManager_(docMgr) {}

std::vector<CodeAction> CodeActionHandler::handleCodeAction(
    const std::string& uri,
    const Range& range,
    const std::vector<Diagnostic>& diagnostics
) {
    std::vector<CodeAction> actions;

    // Get implement interface actions for the current line
    auto implementActions = getImplementInterfaceActions(uri, range.start.line);
    actions.insert(actions.end(), implementActions.begin(), implementActions.end());

    return actions;
}

std::vector<CodeAction> CodeActionHandler::getImplementInterfaceActions(
    const std::string& uri,
    int line
) {
    std::vector<CodeAction> actions;

    auto doc = documentManager_->getDocument(uri);
    if (!doc || !doc->environment) {
        return actions;
    }

    // Get the content of the current line
    std::string content = doc->content;
    std::istringstream stream(content);
    std::string currentLine;
    int currentLineNum = 0;

    while (std::getline(stream, currentLine) && currentLineNum < line) {
        currentLineNum++;
    }

    if (currentLineNum != line) {
        return actions;
    }

    // Simple parsing: look for "class ClassName implements InterfaceName"
    size_t classPos = currentLine.find("class ");
    size_t implementsPos = currentLine.find("implements ");

    if (classPos == std::string::npos || implementsPos == std::string::npos) {
        return actions;
    }

    // Extract class name
    size_t classNameStart = classPos + 6; // "class ".length()
    size_t classNameEnd = currentLine.find_first_of(" \t{", classNameStart);
    if (classNameEnd == std::string::npos) classNameEnd = currentLine.length();
    std::string className = currentLine.substr(classNameStart, classNameEnd - classNameStart);

    // Extract interface name
    size_t interfaceNameStart = implementsPos + 11; // "implements ".length()
    size_t interfaceNameEnd = currentLine.find_first_of(" \t{", interfaceNameStart);
    if (interfaceNameEnd == std::string::npos) interfaceNameEnd = currentLine.length();
    std::string interfaceName = currentLine.substr(interfaceNameStart, interfaceNameEnd - interfaceNameStart);

    // Trim whitespace
    interfaceName.erase(interfaceName.find_last_not_of(" \t\r\n") + 1);

    // Get required methods from interface
    auto requiredMethods = getRequiredMethods(interfaceName, doc);

    if (requiredMethods.empty()) {
        return actions;
    }

    // Generate code action to implement all methods
    CodeAction action;
    action.title = "Implement interface methods";
    action.kind = "quickfix";

    // Build method stubs with @Override annotation
    std::stringstream stubsBuilder;
    for (const auto& methodStub : requiredMethods) {
        stubsBuilder << "    @Override\n";
        stubsBuilder << "    " << methodStub << " {\n";
        stubsBuilder << "        // TODO: Implement method\n";
        stubsBuilder << "    }\n\n";
    }

    // Find the closing brace of the class
    std::istringstream braceStream(doc->content);
    std::string docLine;
    int closingBraceLine = line + 1;
    int braceLineNum = 0;
    int braceDepth = 0;
    bool foundClassBody = false;

    // Start from the class declaration line
    while (std::getline(braceStream, docLine)) {
        if (braceLineNum == line) {
            // This is the class declaration line, start counting braces
            foundClassBody = true;
            // Count opening braces on this line
            for (char c : docLine) {
                if (c == '{') braceDepth++;
                else if (c == '}') braceDepth--;
            }
        } else if (foundClassBody) {
            // Count braces on subsequent lines
            for (char c : docLine) {
                if (c == '{') braceDepth++;
                else if (c == '}') {
                    braceDepth--;
                    if (braceDepth == 0) {
                        // Found the closing brace of the class
                        closingBraceLine = braceLineNum;
                        break;
                    }
                }
            }
            if (braceDepth == 0) break;
        }
        braceLineNum++;
    }

    // Create text edit to insert methods before the closing brace
    WorkspaceEdit edit;
    TextEdit textEdit;
    textEdit.range.start.line = closingBraceLine;
    textEdit.range.start.character = 0;
    textEdit.range.end = textEdit.range.start;
    textEdit.newText = stubsBuilder.str();

    edit.changes[uri].push_back(textEdit);
    action.edit = edit;

    actions.push_back(action);

    return actions;
}

std::vector<std::string> CodeActionHandler::getRequiredMethods(
    const std::string& interfaceName,
    const Document* doc
) {
    std::vector<std::string> methods;

    if (!doc->environment) {
        return methods;
    }

    auto interfaceRegistry = doc->environment->getInterfaceRegistry();
    if (!interfaceRegistry) {
        return methods;
    }

    // Get interface definition
    auto interfaceDef = interfaceRegistry->findInterface(interfaceName);
    if (!interfaceDef) {
        return methods;
    }

    // Get method signatures from interface
    const auto& signatures = interfaceDef->getMethodSignatures();

    for (const auto& sig : signatures) {
        // Build method signature string
        std::stringstream methodBuilder;
        methodBuilder << "function " << sig.name << "(";

        // Add parameters
        bool first = true;
        for (const auto& [paramName, paramType] : sig.parameters) {
            if (!first) methodBuilder << ", ";
            methodBuilder << paramName << ": " << paramType->getBaseTypeName();
            first = false;
        }

        methodBuilder << "): " << sig.returnType->getBaseTypeName();

        methods.push_back(methodBuilder.str());
    }

    return methods;
}

bool CodeActionHandler::classHasMethod(
    const std::string& className,
    const std::string& methodName,
    const Document* doc
) {
    if (!doc->environment) {
        return false;
    }

    auto classRegistry = doc->environment->getClassRegistry();
    if (!classRegistry) {
        return false;
    }

    auto classDef = classRegistry->findClass(className);
    if (!classDef) {
        return false;
    }

    return classDef->hasMethod(methodName);
}

} // namespace mtype::lsp
