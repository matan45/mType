#include "HoverHandler.hpp"
#include <unordered_map>

namespace mtype::lsp {

HoverHandler::HoverHandler(DocumentManager* docMgr)
    : documentManager_(docMgr) {}

std::optional<Hover> HoverHandler::handleHover(const std::string& uri, const Position& position) {
    auto doc = documentManager_->getDocument(uri);
    if (!doc) return std::nullopt;

    // Get word at position
    std::string word = documentManager_->getWordAtPosition(uri, position.line, position.character);
    if (word.empty()) return std::nullopt;

    // Check if it's a keyword
    auto keywordHover = getKeywordHover(word);
    if (keywordHover) return keywordHover;

    // Check if it's a type
    auto typeHover = getTypeHover(word);
    if (typeHover) return typeHover;

    // Check if it's a builtin function
    auto builtinHover = getBuiltinHover(word);
    if (builtinHover) return builtinHover;

    return std::nullopt;
}

std::optional<Hover> HoverHandler::getKeywordHover(const std::string& word) const {
    static const std::unordered_map<std::string, std::string> keywordDocs = {
        {"class", "Defines a new class"},
        {"interface", "Defines a new interface"},
        {"function", "Defines a new function"},
        {"async", "Marks a function as asynchronous, returns Promise<T>"},
        {"await", "Waits for a Promise to resolve"},
        {"extends", "Inherits from a base class"},
        {"implements", "Implements an interface"},
        {"public", "Public access modifier"},
        {"private", "Private access modifier"},
        {"protected", "Protected access modifier"},
        {"static", "Static member (class-level)"},
        {"final", "Prevents inheritance or overriding"},
        {"abstract", "Abstract class or method"},
        {"return", "Returns a value from a function"},
        {"if", "Conditional statement"},
        {"else", "Alternative branch for if statement"},
        {"while", "Loop while condition is true"},
        {"for", "Loop with initialization, condition, and increment"},
        {"foreach", "Iterate over a collection"},
        {"try", "Begin exception handling block"},
        {"catch", "Handle exceptions"},
        {"finally", "Execute code regardless of exceptions"},
        {"throw", "Throw an exception"}
    };

    auto it = keywordDocs.find(word);
    if (it != keywordDocs.end()) {
        Hover hover;
        hover.contents = "**" + word + "** (keyword)\n\n" + it->second;
        return hover;
    }

    return std::nullopt;
}

std::optional<Hover> HoverHandler::getTypeHover(const std::string& word) const {
    static const std::unordered_map<std::string, std::string> typeDocs = {
        {"int", "32-bit signed integer"},
        {"float", "64-bit floating-point number"},
        {"string", "String type"},
        {"bool", "Boolean type (true/false)"},
        {"void", "Void type (no return value)"},
        {"List", "List<T>: Dynamic resizable list"},
        {"LinkedList", "LinkedList<T>: Doubly-linked list"},
        {"Stack", "Stack<T>: LIFO stack"},
        {"Queue", "Queue<T>: FIFO queue"},
        {"HashMap", "HashMap<K,V>: Hash-based map"},
        {"HashSet", "HashSet<T>: Hash-based set"},
        {"Promise", "Promise<T>: Represents an asynchronous operation"}
    };

    auto it = typeDocs.find(word);
    if (it != typeDocs.end()) {
        Hover hover;
        hover.contents = "**" + word + "** (type)\n\n" + it->second;
        return hover;
    }

    return std::nullopt;
}

std::optional<Hover> HoverHandler::getBuiltinHover(const std::string& word) const {
    static const std::unordered_map<std::string, std::string> builtinDocs = {
        {"print", "```mtype\nfunction print(value): void\n```\nPrints a value to the console"},
        {"typeof", "```mtype\nfunction typeof(value): string\n```\nReturns the type name of a value"},
        {"hashCode", "```mtype\nfunction hashCode(value): int\n```\nComputes the hash code of a value"},
        {"parsePrimitive", "```mtype\nfunction parsePrimitive(value): string\n```\nConverts a primitive value to string"},
        {"arrayAdd", "```mtype\nfunction arrayAdd(a, b): array\n```\nElement-wise array addition (SIMD-accelerated)"},
        {"arraySum", "```mtype\nfunction arraySum(arr): number\n```\nSum of all array elements (SIMD-accelerated)"},
        {"arrayMin", "```mtype\nfunction arrayMin(arr): number\n```\nMinimum element in array (SIMD-accelerated)"},
        {"arrayMax", "```mtype\nfunction arrayMax(arr): number\n```\nMaximum element in array (SIMD-accelerated)"}
    };

    auto it = builtinDocs.find(word);
    if (it != builtinDocs.end()) {
        Hover hover;
        hover.contents = it->second;
        return hover;
    }

    return std::nullopt;
}

} // namespace mtype::lsp
