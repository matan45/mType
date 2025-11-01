#include "CompletionHandler.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace mtype::lsp {

CompletionHandler::CompletionHandler(DocumentManager* docMgr)
    : documentManager_(docMgr),
      pathCompletionHandler_(std::make_unique<PathCompletionHandler>(docMgr)) {}

std::vector<CompletionItem> CompletionHandler::handleCompletion(
    const std::string& uri,
    const Position& position
) {
    std::cerr << "[CompletionHandler] handleCompletion called for uri: " << uri
              << " at position (line: " << position.line << ", char: " << position.character << ")" << std::endl;

    std::vector<CompletionItem> items;

    // Check for path completions first (inside import strings)
    auto pathItems = pathCompletionHandler_->getPathCompletions(uri, position);
    if (!pathItems.empty()) {
        std::cerr << "[CompletionHandler] Returning " << pathItems.size() << " path completions" << std::endl;
        // If we're in a path context, only return path completions
        return pathItems;
    }

    // Get document
    auto doc = documentManager_->getDocument(uri);
    if (!doc) {
        std::cerr << "[CompletionHandler] Document not found" << std::endl;
        return items;
    }

    // Get the current line to determine context
    std::string currentLine = getLineAtPosition(doc->content, position);
    std::string textBeforeCursor = currentLine.substr(0, std::min(position.character, static_cast<int>(currentLine.length())));
    std::cerr << "[CompletionHandler] Current line: '" << currentLine << "'" << std::endl;
    std::cerr << "[CompletionHandler] Text before cursor: '" << textBeforeCursor << "'" << std::endl;

    // Check context-specific completions
    // If we're after "implements", only show interfaces
    if (textBeforeCursor.find("implements") != std::string::npos) {
        std::cerr << "[CompletionHandler] Detected 'implements' context - returning only interfaces" << std::endl;
        auto interfaces = getInterfaceCompletions(uri);
        std::cerr << "[CompletionHandler] Found " << interfaces.size() << " interfaces" << std::endl;
        for (const auto& iface : interfaces) {
            std::cerr << "[CompletionHandler]   - " << iface.label << " (kind: " << iface.kind << ")" << std::endl;
        }
        return interfaces;
    }

    // If we're after "extends", only show classes
    if (textBeforeCursor.find("extends") != std::string::npos) {
        std::cerr << "[CompletionHandler] Detected 'extends' context - returning only classes" << std::endl;
        auto classes = getClassCompletions(uri);
        std::cerr << "[CompletionHandler] Found " << classes.size() << " classes" << std::endl;
        for (const auto& cls : classes) {
            std::cerr << "[CompletionHandler]   - " << cls.label << " (kind: " << cls.kind << ")" << std::endl;
        }
        return classes;
    }

    // Otherwise, provide all completions
    std::cerr << "[CompletionHandler] General context - providing all completions" << std::endl;

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

    std::cerr << "[CompletionHandler] Returning " << items.size() << " total completions (keywords: "
              << keywords.size() << ", types: " << types.size() << ", builtins: " << builtins.size()
              << ", collections: " << collections.size() << ", variables: " << variables.size()
              << ", classes: " << classes.size() << ", interfaces: " << interfaces.size() << ")" << std::endl;

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
        "break", "continue", "switch", "case", "default", "do",
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
    std::cerr << "[CompletionHandler::getClassCompletions] Called for uri: " << uri << std::endl;
    std::vector<CompletionItem> items;

    auto doc = documentManager_->getDocument(uri);
    if (!doc || !doc->environment) {
        std::cerr << "[CompletionHandler::getClassCompletions] No document or environment" << std::endl;
        return items;
    }

    // Get classes from the environment's class registry
    auto classRegistry = doc->environment->getClassRegistry();
    if (!classRegistry) {
        std::cerr << "[CompletionHandler::getClassCompletions] No class registry" << std::endl;
        return items;
    }

    // Get ALL registered classes (not just from symbol locations)
    auto classNames = classRegistry->getAllItemNames();
    std::cerr << "[CompletionHandler::getClassCompletions] Found " << classNames.size() << " classes in registry" << std::endl;

    for (const auto& className : classNames) {
        std::cerr << "[CompletionHandler::getClassCompletions]   - Adding class: " << className << std::endl;
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
    std::cerr << "[CompletionHandler::getInterfaceCompletions] Called for uri: " << uri << std::endl;
    std::vector<CompletionItem> items;

    auto doc = documentManager_->getDocument(uri);
    if (!doc || !doc->environment) {
        std::cerr << "[CompletionHandler::getInterfaceCompletions] No document or environment" << std::endl;
        return items;
    }

    // Get interfaces from the environment's interface registry
    auto interfaceRegistry = doc->environment->getInterfaceRegistry();
    if (!interfaceRegistry) {
        std::cerr << "[CompletionHandler::getInterfaceCompletions] No interface registry" << std::endl;
        return items;
    }

    // Get ALL registered interfaces
    auto allInterfaces = interfaceRegistry->getAllInterfaces();
    std::cerr << "[CompletionHandler::getInterfaceCompletions] Found " << allInterfaces.size() << " interfaces in registry" << std::endl;

    for (const auto& [interfaceName, interfaceDef] : allInterfaces) {
        std::cerr << "[CompletionHandler::getInterfaceCompletions]   - Adding interface: " << interfaceName << " (kind: " << static_cast<int>(CompletionItemKind::Interface) << ")" << std::endl;
        CompletionItem item;
        item.label = interfaceName;
        item.kind = static_cast<int>(CompletionItemKind::Interface);
        item.detail = "interface";
        item.insertText = interfaceName;
        items.push_back(item);
    }

    return items;
}

} // namespace mtype::lsp
