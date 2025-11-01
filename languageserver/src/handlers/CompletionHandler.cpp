#include "CompletionHandler.hpp"

namespace mtype::lsp {

CompletionHandler::CompletionHandler(DocumentManager* docMgr)
    : documentManager_(docMgr) {}

std::vector<CompletionItem> CompletionHandler::handleCompletion(
    const std::string& uri,
    const Position& position
) {
    std::vector<CompletionItem> items;

    // Get document
    auto doc = documentManager_->getDocument(uri);
    if (!doc) {
        return items;
    }

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

    return items;
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

} // namespace mtype::lsp
