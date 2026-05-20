#include "CompletionHelpers.hpp"

#include "../../utils/StringUtils.hpp"
#include "../../../../mType/ast/AccessModifier.hpp"
#include "../../../../mType/environment/registry/FieldDefinition.hpp"
#include "../../../../mType/environment/registry/MethodDefinition.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <regex>
#include <sstream>
#include <tuple>
#include <utility>

namespace mtype::lsp::internal
{
    using utils::toLowerAscii;

    int kindPriority(int kind)
    {
        switch (static_cast<CompletionItemKind>(kind)) {
            case CompletionItemKind::Variable: return 1;
            case CompletionItemKind::Field: return 2;
            case CompletionItemKind::Method: return 3;
            case CompletionItemKind::Function: return 4;
            case CompletionItemKind::Class:
            case CompletionItemKind::Interface: return 5;
            case CompletionItemKind::Keyword: return 8;
            default: return 6;
        }
    }

    int matchPriority(const CompletionItem& item, const std::string& typedPrefix)
    {
        if (typedPrefix.empty()) return 2;
        const std::string labelLower = toLowerAscii(item.label);
        const std::string prefixLower = toLowerAscii(typedPrefix);
        if (labelLower == prefixLower) return 0;
        if (labelLower.rfind(prefixLower, 0) == 0) return 1;
        return 2;
    }

    bool isAutoImportItem(const CompletionItem& item)
    {
        return item.detail && item.detail->find("Auto-import") == 0;
    }

    int completionPriority(const CompletionItem& item, const std::string& typedPrefix)
    {
        const int autoImportOffset = isAutoImportItem(item) ? 20 : 0;
        return autoImportOffset + (matchPriority(item, typedPrefix) * 10) + kindPriority(item.kind);
    }

    void applySortText(std::vector<CompletionItem>& items, const std::string& typedPrefix)
    {
        std::size_t stableIndex = 0;
        for (auto& item : items) {
            std::ostringstream sort;
            sort << std::setw(2) << std::setfill('0')
                 << completionPriority(item, typedPrefix) << "_"
                 << toLowerAscii(item.label) << "_"
                 << std::setw(4) << std::setfill('0') << stableIndex++;
            item.sortText = sort.str();
            if (!item.filterText) {
                item.filterText = item.label;
            }
        }
    }

    void sortCompletions(std::vector<CompletionItem>& items, const std::string& typedPrefix)
    {
        applySortText(items, typedPrefix);
        std::stable_sort(items.begin(), items.end(),
            [](const CompletionItem& a, const CompletionItem& b) {
                return a.sortText.value_or(a.label) < b.sortText.value_or(b.label);
            });
    }

    void applyReplacementEdit(std::vector<CompletionItem>& items,
                              const Position& position,
                              const std::string& typedPrefix)
    {
        if (typedPrefix.empty()) return;

        const int startCharacter =
            std::max(0, position.character - static_cast<int>(typedPrefix.size()));
        for (auto& item : items) {
            TextEdit edit;
            edit.range = {{position.line, startCharacter}, position};
            edit.newText = item.insertText.value_or(item.label);
            item.textEdit = std::move(edit);
        }
    }

    CompletionItemKind workspaceKindToCompletionKind(analysis::WorkspaceSymbolKind kind)
    {
        switch (kind) {
            case analysis::WorkspaceSymbolKind::Function:
                return CompletionItemKind::Function;
            case analysis::WorkspaceSymbolKind::Interface:
                return CompletionItemKind::Interface;
            case analysis::WorkspaceSymbolKind::Class:
                return CompletionItemKind::Class;
            case analysis::WorkspaceSymbolKind::Unknown:
            default:
                return CompletionItemKind::Reference;
        }
    }

    std::string formatParameters(
        const std::vector<std::pair<std::string, value::ParameterType>>& params)
    {
        std::string text;
        for (std::size_t i = 0; i < params.size(); ++i) {
            if (i > 0) text += ", ";
            text += params[i].second.toString();
            text += " ";
            text += params[i].first;
        }
        return text;
    }

    std::string valueTypeToString(value::ValueType type)
    {
        switch (type) {
            case value::ValueType::INT: return "int";
            case value::ValueType::FLOAT: return "float";
            case value::ValueType::BOOL: return "bool";
            case value::ValueType::STRING: return "string";
            case value::ValueType::VOID: return "void";
            case value::ValueType::OBJECT: return "object";
            case value::ValueType::ARRAY: return "array";
            case value::ValueType::LAMBDA: return "lambda";
            case value::ValueType::NULL_TYPE: return "null";
            case value::ValueType::PROMISE: return "Promise";
            default: return "unknown";
        }
    }

    std::string methodDetail(
        const std::shared_ptr<runtimeTypes::klass::MethodDefinition>& method,
        const std::string& owner,
        std::size_t overloadCount,
        bool isStatic)
    {
        std::string detail = std::string(ast::accessModifierToString(method->getAccessModifier())) + " ";
        if (isStatic) detail += "static ";
        detail += owner + "." + method->getName() + "(";
        detail += formatParameters(method->getParametersWithTypes());
        detail += "): ";
        detail += valueTypeToString(method->getReturnType());
        if (overloadCount > 1) {
            detail += " (" + std::to_string(overloadCount) + " overloads)";
        }
        return detail;
    }

    std::string fieldDetail(
        const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field,
        const std::string& owner,
        bool isStatic)
    {
        std::string detail = std::string(ast::accessModifierToString(field->getAccessModifier())) + " ";
        if (isStatic) detail += "static ";
        detail += owner + " field: ";
        detail += valueTypeToString(field->getType());
        return detail;
    }

    // Builds a callable snippet body so accepting a function/method
    // completion drops the user inside the parameter list with the
    // first argument selected. Empty-param methods become "name()$0".
    std::string buildCallSnippet(
        const std::string& name,
        const std::vector<std::pair<std::string, value::ParameterType>>& params)
    {
        std::string snippet = name + "(";
        for (std::size_t i = 0; i < params.size(); ++i) {
            if (i > 0) snippet += ", ";
            const std::string& paramName = params[i].first;
            snippet += "${" + std::to_string(i + 1) + ":";
            snippet += paramName.empty() ? ("p" + std::to_string(i + 1)) : paramName;
            snippet += "}";
        }
        snippet += ")$0";
        return snippet;
    }

    bool cursorFollowedByOpenParen(const std::string& textAfterCursor)
    {
        for (char c : textAfterCursor) {
            if (c == '(') return true;
            if (!std::isspace(static_cast<unsigned char>(c))) return false;
        }
        return false;
    }

    void stampResolveData(CompletionItem& item,
                          const std::string& uri,
                          const std::string& kind,
                          const std::string& owner,
                          const std::string& name)
    {
        json data = {
            {"uri", uri},
            {"kind", kind},
            {"name", name}
        };
        if (!owner.empty()) data["owner"] = owner;
        item.data = std::move(data);
    }

    void attachCallCommitChars(CompletionItem& item)
    {
        item.commitCharacters = {"("};
    }

    bool isCallableKind(int kind)
    {
        const auto k = static_cast<CompletionItemKind>(kind);
        return k == CompletionItemKind::Method
            || k == CompletionItemKind::Function
            || k == CompletionItemKind::Constructor;
    }

    bool textEndsInAnnotationTrigger(const std::string& text, std::string& outPartial)
    {
        try {
            std::regex re(R"((^|[^A-Za-z0-9_])@(\w*)$)");
            std::smatch match;
            if (std::regex_search(text, match, re)) {
                outPartial = match[2].matched ? match[2].str() : std::string{};
                return true;
            }
            return false;
        } catch (const std::regex_error&) {
            return false;
        }
    }

    // Matches `extends`/`implements` used as a token (word-boundary on
    // both sides), optionally followed by whitespace and a partial
    // identifier. `outPartial` returns the identifier the user typed
    // after the keyword, so callers fuzzy-filter with it rather than
    // with the keyword itself.
    bool textEndsInInheritanceKeyword(const std::string& text,
                                      const char* keyword,
                                      std::string& outPartial)
    {
        const std::string pattern =
            std::string("(^|[^A-Za-z0-9_])") + keyword + "(\\s+(\\w*))?$";
        try {
            std::regex re(pattern);
            std::smatch match;
            if (std::regex_search(text, match, re)) {
                outPartial = match[3].matched ? match[3].str() : std::string{};
                return true;
            }
            return false;
        } catch (const std::regex_error&) {
            return false;
        }
    }

    // Caveat: brace counter doesn't recognise braces inside string
    // literals or comments — a line like `string s = "{ token";` will
    // perturb braceDepth and may mis-attribute the enclosing class for
    // that span. Documented limitation, not a correctness bug.
    std::vector<CompletionItem> keywordCompletions()
    {
        // Type-system keywords previously missing from the dropdown:
        // `annotation` declares user-defined annotation types,
        // `value` marks value classes used by lib/primitives wrappers,
        // `isClassOf` is the runtime type-test operator.
        static const std::vector<std::string> kKeywords = {
            "class", "interface", "function", "if", "else", "while", "for", "foreach",
            "return", "new", "this", "super", "extends", "implements",
            "public", "private", "protected", "static", "final", "abstract",
            "async", "await", "try", "catch", "finally", "throw",
            "break", "continue", "switch", "case", "default", "do", "match",
            "import", "from", "null", "true", "false",
            "annotation", "value", "isClassOf"
        };
        std::vector<CompletionItem> items;
        items.reserve(kKeywords.size());
        for (const auto& kw : kKeywords) {
            CompletionItem item;
            item.label = kw;
            item.kind = static_cast<int>(CompletionItemKind::Keyword);
            item.detail = "keyword";
            item.insertText = kw;
            items.push_back(std::move(item));
        }
        return items;
    }

    std::vector<CompletionItem> builtinCompletions()
    {
        static const std::vector<std::tuple<std::string, std::string, std::string>> kBuiltins = {
            {"print", "print(value): void", "print(${1:value})"},
            {"typeof", "typeof(value): string", "typeof(${1:value})"},
            {"hashCode", "hashCode(value): int", "hashCode(${1:value})"},
            {"parsePrimitive", "parsePrimitive(value): string", "parsePrimitive(${1:value})"},
            {"arrayAdd", "arrayAdd(a, b): array", "arrayAdd(${1:a}, ${2:b})"},
            {"arraySum", "arraySum(arr): number", "arraySum(${1:arr})"},
            {"arrayMin", "arrayMin(arr): number", "arrayMin(${1:arr})"},
            {"arrayMax", "arrayMax(arr): number", "arrayMax(${1:arr})"}
        };
        std::vector<CompletionItem> items;
        items.reserve(kBuiltins.size());
        for (const auto& [name, signature, snippet] : kBuiltins) {
            CompletionItem item;
            item.label = name;
            item.kind = static_cast<int>(CompletionItemKind::Function);
            item.detail = signature;
            item.insertText = snippet;
            item.insertTextFormat = kSnippetInsertTextFormat;
            item.filterText = name;
            items.push_back(std::move(item));
        }
        return items;
    }

    std::vector<CompletionItem> collectionCompletions()
    {
        static const std::vector<std::pair<std::string, std::string>> kCollections = {
            {"List", "List<T>: dynamic resizable list"},
            {"LinkedList", "LinkedList<T>: doubly-linked list"},
            {"Stack", "Stack<T>: LIFO stack"},
            {"Queue", "Queue<T>: FIFO queue"},
            {"HashMap", "HashMap<K,V>: hash-based map"},
            {"HashSet", "HashSet<T>: hash-based set"}
        };
        std::vector<CompletionItem> items;
        items.reserve(kCollections.size());
        for (const auto& [name, description] : kCollections) {
            CompletionItem item;
            item.label = name;
            item.kind = static_cast<int>(CompletionItemKind::Class);
            item.detail = "collection type";
            item.documentation = description;
            item.insertText = name;
            items.push_back(std::move(item));
        }
        return items;
    }

    std::string inferEnclosingClass(const std::string& content, int targetLine)
    {
        static const std::regex classRegex(
            R"(\bclass\s+([A-Za-z_][A-Za-z0-9_]*))");
        std::istringstream stream(content);
        std::string line;
        int currentLine = 0;
        int braceDepth = 0;
        std::string currentClass;

        while (std::getline(stream, line) && currentLine <= targetLine) {
            std::smatch match;
            if (std::regex_search(line, match, classRegex)) {
                currentClass = match[1].str();
            }

            for (char c : line) {
                if (c == '{') ++braceDepth;
                else if (c == '}') {
                    --braceDepth;
                    if (braceDepth <= 0) {
                        braceDepth = 0;
                        if (currentLine < targetLine) currentClass.clear();
                    }
                }
            }
            ++currentLine;
        }

        return currentClass;
    }
}
