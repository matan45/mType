#include "CompletionHandler.hpp"
#include "../../../mType/ast/AccessModifier.hpp"
#include <sstream>
#include <algorithm>

namespace mtype::lsp {

CompletionHandler::CompletionHandler(DocumentManager* docMgr)
    : documentManager_(docMgr),
      pathCompletionHandler_(std::make_unique<PathCompletionHandler>(docMgr)) {}

std::vector<CompletionItem> CompletionHandler::handleCompletion(
    const std::string& uri,
    const Position& position
) {
    std::vector<CompletionItem> items;

    // Check for path completions first (inside import strings)
    auto pathItems = pathCompletionHandler_->getPathCompletions(uri, position);
    if (!pathItems.empty()) {
        // If we're in a path context, only return path completions
        return pathItems;
    }

    // Get document
    auto doc = documentManager_->getDocument(uri);
    if (!doc) {
        return items;
    }

    // Get the current line to determine context
    std::string currentLine = getLineAtPosition(doc->content, position);
    std::string textBeforeCursor = currentLine.substr(0, std::min(position.character, static_cast<int>(currentLine.length())));

    // Check if we're after :: (static member access) or . (instance member access)
    // Trim whitespace from the end
    std::string trimmed = textBeforeCursor;
    while (!trimmed.empty() && std::isspace(trimmed.back())) {
        trimmed.pop_back();
    }

    // Check for :: (static member access like ClassName::staticMethod)
    bool isStaticAccess = false;
    size_t operatorPos = std::string::npos;

    if (trimmed.length() >= 2 && trimmed.substr(trimmed.length() - 2) == "::") {
        isStaticAccess = true;
        operatorPos = trimmed.length() - 2;
    }
    // Check for . (instance member access like object.method)
    else if (!trimmed.empty() && trimmed.back() == '.') {
        isStaticAccess = false;
        operatorPos = trimmed.length() - 1;
    }

    if (operatorPos != std::string::npos) {
        // We're in member access context - find the identifier before :: or .
        size_t identifierStart = operatorPos;

        // Go backwards to find the start of the identifier
        while (identifierStart > 0 &&
               (std::isalnum(trimmed[identifierStart - 1]) || trimmed[identifierStart - 1] == '_')) {
            identifierStart--;
        }

        std::string identifier = trimmed.substr(identifierStart, operatorPos - identifierStart);

        if (!identifier.empty()) {
            // Get member completions for this identifier
            auto memberItems = getMemberCompletions(uri, identifier, position.line, isStaticAccess);
            return memberItems;
        }
    }

    // Check context-specific completions
    // If we're after "implements", only show interfaces
    if (textBeforeCursor.find("implements") != std::string::npos) {
        auto interfaces = getInterfaceCompletions(uri);
        return interfaces;
    }

    // If we're after "extends", only show classes
    if (textBeforeCursor.find("extends") != std::string::npos) {
        auto classes = getClassCompletions(uri);
        return classes;
    }

    // Otherwise, provide all completions

    // Add keyword completions
    auto keywords = getKeywordCompletions();
    items.insert(items.end(), keywords.begin(), keywords.end());

    // Add type completions
    auto types = getTypeCompletions();
    items.insert(items.end(), types.begin(), types.end());

    // Add builtin functions
    auto builtins = getBuiltinCompletions();
    items.insert(items.end(), builtins.begin(), builtins.end());

    // Add collection types
    auto collections = getCollectionCompletions();
    items.insert(items.end(), collections.begin(), collections.end());

    // Add variables from current document
    auto variables = getVariableCompletions(uri, position.line);
    items.insert(items.end(), variables.begin(), variables.end());

    // Add classes from environment
    auto classes = getClassCompletions(uri);
    items.insert(items.end(), classes.begin(), classes.end());

    // Add interfaces from environment
    auto interfaces = getInterfaceCompletions(uri);
    items.insert(items.end(), interfaces.begin(), interfaces.end());

    return items;
}

std::string CompletionHandler::getLineAtPosition(const std::string& content, const Position& position) const {
    std::istringstream stream(content);
    std::string line;
    int currentLine = 0;

    while (std::getline(stream, line) && currentLine < position.line) {
        currentLine++;
    }

    if (currentLine == position.line) {
        return line;
    }

    return "";
}

std::vector<CompletionItem> CompletionHandler::getKeywordCompletions() const {
    std::vector<CompletionItem> items;

    std::vector<std::string> keywords = {
        "class", "interface", "function", "if", "else", "while", "for", "foreach",
        "return", "new", "this", "super", "extends", "implements",
        "public", "private", "protected", "static", "final", "abstract",
        "async", "await", "try", "catch", "finally", "throw",
        "break", "continue", "switch", "case", "default", "do", "match",
        "import", "from", "null", "true", "false"
    };

    for (const auto& kw : keywords) {
        CompletionItem item;
        item.label = kw;
        item.kind = static_cast<int>(CompletionItemKind::Keyword);
        item.detail = "keyword";
        item.insertText = kw;
        items.push_back(item);
    }

    return items;
}

std::vector<CompletionItem> CompletionHandler::getTypeCompletions() const {
    std::vector<CompletionItem> items;

    std::vector<std::string> types = {
        "int", "float", "string", "bool", "void"
    };

    for (const auto& type : types) {
        CompletionItem item;
        item.label = type;
        item.kind = static_cast<int>(CompletionItemKind::Class);
        item.detail = "primitive type";
        item.insertText = type;
        items.push_back(item);
    }

    // Wrapper types
    std::vector<std::string> wrappers = {
        "Int", "Float", "String", "Bool", "Promise"
    };

    for (const auto& wrapper : wrappers) {
        CompletionItem item;
        item.label = wrapper;
        item.kind = static_cast<int>(CompletionItemKind::Class);
        item.detail = "wrapper type";
        item.insertText = wrapper;
        items.push_back(item);
    }

    return items;
}

std::vector<CompletionItem> CompletionHandler::getBuiltinCompletions() const {
    std::vector<CompletionItem> items;

    std::vector<std::pair<std::string, std::string>> builtins = {
        {"print", "print(value): void"},
        {"typeof", "typeof(value): string"},
        {"hashCode", "hashCode(value): int"},
        {"parsePrimitive", "parsePrimitive(value): string"},
        {"arrayAdd", "arrayAdd(a, b): array"},
        {"arraySum", "arraySum(arr): number"},
        {"arrayMin", "arrayMin(arr): number"},
        {"arrayMax", "arrayMax(arr): number"}
    };

    for (const auto& [name, signature] : builtins) {
        CompletionItem item;
        item.label = name;
        item.kind = static_cast<int>(CompletionItemKind::Function);
        item.detail = signature;
        item.insertText = name;
        items.push_back(item);
    }

    return items;
}

std::vector<CompletionItem> CompletionHandler::getCollectionCompletions() const {
    std::vector<CompletionItem> items;

    std::vector<std::pair<std::string, std::string>> collections = {
        {"List", "List<T>: dynamic resizable list"},
        {"LinkedList", "LinkedList<T>: doubly-linked list"},
        {"Stack", "Stack<T>: LIFO stack"},
        {"Queue", "Queue<T>: FIFO queue"},
        {"HashMap", "HashMap<K,V>: hash-based map"},
        {"HashSet", "HashSet<T>: hash-based set"}
    };

    for (const auto& [name, description] : collections) {
        CompletionItem item;
        item.label = name;
        item.kind = static_cast<int>(CompletionItemKind::Class);
        item.detail = "collection type";
        item.documentation = description;
        item.insertText = name;
        items.push_back(item);
    }

    return items;
}

std::vector<CompletionItem> CompletionHandler::getVariableCompletions(const std::string& uri, int line) const {
    std::vector<CompletionItem> items;

    auto identifiers = documentManager_->getIdentifiersInScope(uri, line);

    for (const auto& id : identifiers) {
        CompletionItem item;
        item.label = id;
        item.kind = static_cast<int>(CompletionItemKind::Variable);
        item.detail = "variable";
        item.insertText = id;
        items.push_back(item);
    }

    return items;
}

std::vector<CompletionItem> CompletionHandler::getClassCompletions(const std::string& uri) const {
    std::vector<CompletionItem> items;

    auto doc = documentManager_->getDocument(uri);
    if (!doc || !doc->environment) {
        return items;
    }

    // Get classes from the environment's class registry
    auto classRegistry = doc->environment->getClassRegistry();
    if (!classRegistry) {
        return items;
    }

    // Get ALL registered classes (not just from symbol locations)
    auto classNames = classRegistry->getAllItemNames();

    for (const auto& className : classNames) {
        CompletionItem item;
        item.label = className;
        item.kind = static_cast<int>(CompletionItemKind::Class);
        item.detail = "class";
        item.insertText = className;
        items.push_back(item);
    }

    return items;
}

std::vector<CompletionItem> CompletionHandler::getInterfaceCompletions(const std::string& uri) const {
    std::vector<CompletionItem> items;

    auto doc = documentManager_->getDocument(uri);
    if (!doc || !doc->environment) {
        return items;
    }

    // Get interfaces from the environment's interface registry
    auto interfaceRegistry = doc->environment->getInterfaceRegistry();
    if (!interfaceRegistry) {
        return items;
    }

    // Get ALL registered interfaces
    auto allInterfaces = interfaceRegistry->getAllInterfaces();

    for (const auto& [interfaceName, interfaceDef] : allInterfaces) {
        CompletionItem item;
        item.label = interfaceName;
        item.kind = static_cast<int>(CompletionItemKind::Interface);
        item.detail = "interface";
        item.insertText = interfaceName;
        items.push_back(item);
    }

    return items;
}

std::vector<CompletionItem> CompletionHandler::getMemberCompletions(
    const std::string& uri,
    const std::string& objectName,
    int line,
    bool isStaticAccess
) const {
    std::vector<CompletionItem> items;

    auto doc = documentManager_->getDocument(uri);
    if (!doc || !doc->environment) {
        return items;
    }

    auto classRegistry = doc->environment->getClassRegistry();
    if (!classRegistry) {
        return items;
    }

    // If using :: operator, ONLY allow static member access from class names
    if (isStaticAccess) {
        // objectName must be a class name for :: operator
        if (classRegistry->hasClass(objectName)) {
            auto classDef = classRegistry->findClass(objectName);
            if (classDef) {
                // Get static methods
                const auto& staticMethods = classDef->getStaticMethods();
                for (const auto& pair : staticMethods) {
                    const std::string& methodName = pair.first;
                    const auto& methodOverloads = pair.second;
                    if (methodOverloads.empty()) continue;

                    // Use first overload for access modifier info
                    const auto& firstOverload = methodOverloads.front();

                    CompletionItem item;
                    item.label = methodName;
                    item.kind = static_cast<int>(CompletionItemKind::Method);

                    // Include access modifier and static keyword
                    std::string detail = std::string(ast::accessModifierToString(firstOverload->getAccessModifier())) + " static method";
                    if (methodOverloads.size() > 1) {
                        detail += " (" + std::to_string(methodOverloads.size()) + " overloads)";
                    }
                    item.detail = detail;
                    item.insertText = methodName;
                    items.push_back(item);
                }

                // Get static fields
                const auto& staticFields = classDef->getStaticFields();
                for (const auto& pair : staticFields) {
                    const std::string& fieldName = pair.first;
                    const auto& fieldDef = pair.second;

                    CompletionItem item;
                    item.label = fieldName;
                    item.kind = static_cast<int>(CompletionItemKind::Field);

                    // Include access modifier and static keyword
                    std::string detail = std::string(ast::accessModifierToString(fieldDef->getAccessModifier())) + " static field";
                    item.detail = detail;
                    item.insertText = fieldName;
                    items.push_back(item);
                }
            }
        }
        return items;
    }

    // If using . operator, provide instance member access
    // Use type inference to show only relevant members for the specific object type
    std::string varType = documentManager_->getVariableType(uri, objectName, line);

    if (varType.empty()) {
        // Fallback: if type inference fails, show all instance members
        auto allClasses = classRegistry->getAllItemNames();
        for (const auto& className : allClasses) {
            auto classDef = classRegistry->findClass(className);
            if (!classDef) continue;

            // Add instance methods from all classes
            const auto& instanceMethods = classDef->getInstanceMethods();
            for (const auto& pair : instanceMethods) {
                const std::string& methodName = pair.first;
                const auto& methodOverloads = pair.second;
                if (methodOverloads.empty()) continue;

                // Use first overload for access modifier info
                const auto& firstOverload = methodOverloads.front();

                CompletionItem item;
                item.label = methodName;
                item.kind = static_cast<int>(CompletionItemKind::Method);

                // Include access modifier
                std::string detail = std::string(ast::accessModifierToString(firstOverload->getAccessModifier())) + " method from " + className;
                if (methodOverloads.size() > 1) {
                    detail += " (" + std::to_string(methodOverloads.size()) + " overloads)";
                }
                item.detail = detail;
                item.insertText = methodName;
                items.push_back(item);
            }

            // Add instance fields from all classes
            const auto& instanceFields = classDef->getInstanceFields();
            for (const auto& pair : instanceFields) {
                const std::string& fieldName = pair.first;
                const auto& fieldDef = pair.second;

                CompletionItem item;
                item.label = fieldName;
                item.kind = static_cast<int>(CompletionItemKind::Field);

                // Include access modifier
                std::string detail = std::string(ast::accessModifierToString(fieldDef->getAccessModifier())) + " field from " + className;
                item.detail = detail;
                item.insertText = fieldName;
                items.push_back(item);
            }
        }
        return items;
    }

    // Type inference succeeded! Show only members from the specific type
    if (!classRegistry->hasClass(varType)) {
        return items; // Type not found in registry
    }

    auto classDef = classRegistry->findClass(varType);
    if (!classDef) {
        return items;
    }

    // Add instance methods from the specific class
    const auto& instanceMethods = classDef->getInstanceMethods();
    for (const auto& pair : instanceMethods) {
        const std::string& methodName = pair.first;
        const auto& methodOverloads = pair.second;
        if (methodOverloads.empty()) continue;

        // Use first overload for access modifier info
        const auto& firstOverload = methodOverloads.front();

        CompletionItem item;
        item.label = methodName;
        item.kind = static_cast<int>(CompletionItemKind::Method);

        // Include access modifier
        std::string detail = std::string(ast::accessModifierToString(firstOverload->getAccessModifier())) + " " + varType + " method";
        if (methodOverloads.size() > 1) {
            detail += " (" + std::to_string(methodOverloads.size()) + " overloads)";
        }
        item.detail = detail;
        item.insertText = methodName;
        items.push_back(item);
    }

    // Add instance fields from the specific class
    const auto& instanceFields = classDef->getInstanceFields();
    for (const auto& pair : instanceFields) {
        const std::string& fieldName = pair.first;
        const auto& fieldDef = pair.second;

        CompletionItem item;
        item.label = fieldName;
        item.kind = static_cast<int>(CompletionItemKind::Field);

        // Include access modifier
        std::string detail = std::string(ast::accessModifierToString(fieldDef->getAccessModifier())) + " " + varType + " field";
        item.detail = detail;
        item.insertText = fieldName;
        items.push_back(item);
    }

    return items;
}

} // namespace mtype::lsp
