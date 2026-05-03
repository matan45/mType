#include "CompletionHandler.hpp"

#include "../analysis/WorkspaceSymbolIndex.hpp"
#include "../utils/ImportUtils.hpp"
#include "../utils/UriUtils.hpp"
#include "../../../mType/ast/AccessModifier.hpp"
#include "../../../mType/diagnostics/IdentifierEnumerator.hpp"
#include "../../../mType/util/EditDistance.hpp"
#include "../../../mType/util/ImportScan.hpp"
#include "../../../mType/util/TokenExtraction.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <regex>
#include <sstream>
#include <tuple>
#include <unordered_set>

namespace mtype::lsp {

namespace {
    constexpr int kSnippetInsertTextFormat = 2;

    std::string toLowerAscii(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
        return text;
    }

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

    void sortCompletions(std::vector<CompletionItem>& items,
                         const std::string& typedPrefix)
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

    CompletionItemKind workspaceKindToCompletionKind(
        analysis::WorkspaceSymbolKind kind)
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
    // first argument selected. Mirrors the form already used for
    // builtins ("print(${1:value})"). Empty-param methods become
    // "name()$0" so the user lands past the closing paren.
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

    // True when the user's cursor is immediately followed by `(`.
    // We then suppress the snippet body so the dropdown doesn't
    // produce `myFunc(${1:p1})()` against an existing call site —
    // the textEdit only replaces the typed prefix, not the trailing
    // parens. Whitespace before the paren still counts as "already
    // a call" for this purpose.
    bool cursorFollowedByOpenParen(const std::string& textAfterCursor)
    {
        for (char c : textAfterCursor) {
            if (c == '(') return true;
            if (!std::isspace(static_cast<unsigned char>(c))) return false;
        }
        return false;
    }

    // Stamp `data` so completionItem/resolve can rehydrate the symbol
    // identity without the server having to recompute the whole
    // completion list. Kept as a tiny json blob — the spec lets us
    // round-trip arbitrary structured data through the client.
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

    // True when the cursor sits right after `@`, optionally followed by
    // a partial annotation name (`@`, `@Ov`, `@Override`). The post-`@`
    // partial is returned via `outPartial` so the caller can fuzzy-filter
    // and replace just the typed name, not the `@` itself.
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

    std::string inferEnclosingClass(const std::string& content, int targetLine)
    {
        std::regex classRegex(R"(\bclass\s+([A-Za-z_][A-Za-z0-9_]*))");
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

    // Returns true if `text` ends in the given keyword used as an
    // inheritance keyword (whitespace-bounded), optionally followed by a
    // partial identifier the user is currently typing. Replaces the
    // earlier `find("extends")` / `find("implements")` checks, which
    // matched substrings inside identifiers (e.g. `myImplementsFoo`)
    // and inside `// implements` comments preceding the cursor.
    // Returns true if `text` ends with the inheritance keyword, optionally
    // followed by whitespace and a partial identifier. When it matches,
    // `outPartial` is set to the partial identifier the user typed AFTER
    // the keyword (may be empty), so the caller can fuzzy-filter with it
    // instead of the global typedPrefix (which would be the keyword itself
    // when the cursor sits right after "implements" or "extends").
    //
    // Matches these cursor positions (| = cursor):
    //   "class X implements|"    → outPartial = ""
    //   "class X implements |"   → outPartial = ""
    //   "class X implements IF|" → outPartial = "IF"
    bool textEndsInInheritanceKeyword(const std::string& text, const char* keyword,
                                      std::string& outPartial)
    {
        // Pattern: word-boundary before keyword, keyword at end, optionally
        // followed by whitespace and a partial identifier.
        const std::string pattern =
            std::string("(^|[^A-Za-z0-9_])") + keyword + "(\\s+(\\w*))?$";
        try {
            std::regex re(pattern);
            std::smatch match;
            if (std::regex_search(text, match, re)) {
                // Group 3 is the partial identifier after the keyword+space.
                // Empty when cursor is right after the keyword or the space.
                outPartial = match[3].matched ? match[3].str() : std::string{};
                return true;
            }
            return false;
        } catch (const std::regex_error&) {
            return false;
        }
    }
}

CompletionHandler::CompletionHandler(
    DocumentManager* docMgr,
    std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex)
    : documentManager_(docMgr),
      workspaceIndex_(std::move(workspaceIndex)),
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
    std::string textBeforeCursor = currentLine.substr(
        0, std::min(position.character, static_cast<int>(currentLine.length())));
    std::string textAfterCursor = (position.character < static_cast<int>(currentLine.length()))
        ? currentLine.substr(position.character)
        : std::string{};

    // MYT-51 — extract the partial identifier under the cursor once,
    // upfront, so every branch can apply the same fuzzy filter.
    const std::string typedPrefix =
        util::extractIdentifierTokenBefore(currentLine, position.character);

    // True when the cursor is already immediately followed by `(` —
    // in that case the call snippet `name(${1:p})$0` would produce a
    // double-paren `name(${1:p})()`. We post-process below to drop
    // back to the bare identifier when this is detected.
    const bool followedByOpenParen = cursorFollowedByOpenParen(textAfterCursor);

    auto stripCallSnippetsIfNeeded = [&](std::vector<CompletionItem>& items) {
        if (!followedByOpenParen) return;
        for (auto& item : items) {
            if (!isCallableKind(item.kind)) continue;
            if (item.insertTextFormat.value_or(1) != kSnippetInsertTextFormat) continue;
            item.insertText = item.label;
            item.insertTextFormat.reset();
        }
    };

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
    // MYT-51 — if we're mid-identifier immediately after `<ident>.` or
    // `<ident>::`, `typedPrefix` will have eaten the partial token but
    // `operatorPos` above still points past it. Recompute by stripping
    // the typed prefix from `trimmed` so the member-access branch can
    // still find the operator.
    else if (!typedPrefix.empty()
             && trimmed.length() > typedPrefix.length()) {
        const std::string beforeTyped =
            trimmed.substr(0, trimmed.length() - typedPrefix.length());
        if (beforeTyped.length() >= 2
            && beforeTyped.substr(beforeTyped.length() - 2) == "::") {
            isStaticAccess = true;
            operatorPos = beforeTyped.length() - 2;
            trimmed = beforeTyped;
        } else if (!beforeTyped.empty() && beforeTyped.back() == '.') {
            isStaticAccess = false;
            operatorPos = beforeTyped.length() - 1;
            trimmed = beforeTyped;
        }
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
            applyFuzzyFilter(memberItems, typedPrefix);
            stripCallSnippetsIfNeeded(memberItems);
            applyReplacementEdit(memberItems, position, typedPrefix);
            sortCompletions(memberItems, typedPrefix);
            return memberItems;
        }
    }

    // Annotation context: cursor sits right after `@` (with optional
    // partial name). Replace just the partial — the `@` itself stays.
    std::string annotationPartial;
    if (textEndsInAnnotationTrigger(textBeforeCursor, annotationPartial)) {
        auto annotations = getAnnotationCompletions(uri);
        applyFuzzyFilter(annotations, annotationPartial);
        applyReplacementEdit(annotations, position, annotationPartial);
        sortCompletions(annotations, annotationPartial);
        return annotations;
    }

    // Check context-specific completions
    // If we're after "implements" (as a token, not a substring), only show
    // interfaces. Token-aware to avoid matching `myImplementsFoo` and
    // `// implements ...` in comments.
    // Use the post-keyword partial (not the global typedPrefix, which
    // would be "implements" itself when the cursor is right after it).
    std::string inheritancePartial;
    if (textEndsInInheritanceKeyword(textBeforeCursor, "implements", inheritancePartial)) {
        auto interfaces = getInterfaceCompletions(uri);
        applyFuzzyFilter(interfaces, inheritancePartial);
        applyReplacementEdit(interfaces, position, inheritancePartial);
        sortCompletions(interfaces, inheritancePartial);
        return interfaces;
    }

    // If we're after "extends" (as a token), only show classes.
    if (textEndsInInheritanceKeyword(textBeforeCursor, "extends", inheritancePartial)) {
        auto classes = getClassCompletions(uri);
        applyFuzzyFilter(classes, inheritancePartial);
        applyReplacementEdit(classes, position, inheritancePartial);
        sortCompletions(classes, inheritancePartial);
        return classes;
    }

    // Otherwise, provide all completions

    // Add keyword completions
    auto keywords = getKeywordCompletions();
    items.insert(items.end(), keywords.begin(), keywords.end());

    // Add type completions
    auto types = getTypeCompletions(uri);
    items.insert(items.end(), types.begin(), types.end());

    // Add builtin functions
    auto builtins = getBuiltinCompletions();
    items.insert(items.end(), builtins.begin(), builtins.end());

    // Add collection types
    auto collections = getCollectionCompletions();
    items.insert(items.end(), collections.begin(), collections.end());

    // MYT-51 — unified identifier enumeration replaces the three
    // per-category walks (variables / classes / interfaces) with a
    // single pass that rides on diagnostics::IdentifierEnumerator
    // and the interface registry, each tagged with the right
    // CompletionItemKind.
    auto identifiers = getIdentifierCompletions(uri);
    items.insert(items.end(), identifiers.begin(), identifiers.end());

    // MYT-51 — apply the fuzzy filter over the in-scope pool before
    // surfacing auto-import items so workspace matches aren't fighting
    // against in-scope matches for the same typed prefix.
    applyFuzzyFilter(items, typedPrefix);

    // MYT-51 — auto-import completion items. No-op if workspaceIndex_
    // is null or the user hasn't typed a prefix yet.
    auto autoImports = getAutoImportCompletions(uri, typedPrefix, items);
    items.insert(items.end(), autoImports.begin(), autoImports.end());

    stripCallSnippetsIfNeeded(items);
    applyReplacementEdit(items, position, typedPrefix);
    sortCompletions(items, typedPrefix);

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

void CompletionHandler::applyFuzzyFilter(
    std::vector<CompletionItem>& items,
    const std::string& typedPrefix) const
{
    if (typedPrefix.empty()) return;

    const int budget = std::max(1, static_cast<int>(typedPrefix.size()) / 3);
    const std::string loweredPrefix = toLowerAscii(typedPrefix);

    items.erase(std::remove_if(items.begin(), items.end(),
        [&loweredPrefix, budget](const CompletionItem& item) {
            const std::string loweredLabel = toLowerAscii(item.label);
            // Keep prefix matches unconditionally — VS Code will
            // re-score them and put them at the top of the list.
            if (loweredLabel.rfind(loweredPrefix, 0) == 0) return false;
            // Otherwise drop anything outside the rustc-style budget.
            return util::levenshtein(loweredPrefix, loweredLabel, budget) > budget;
        }),
        items.end());
}

std::vector<CompletionItem> CompletionHandler::getKeywordCompletions() const {
    std::vector<CompletionItem> items;

    std::vector<std::string> keywords = {
        "class", "interface", "function", "if", "else", "while", "for", "foreach",
        "return", "new", "this", "super", "extends", "implements",
        "public", "private", "protected", "static", "final", "abstract",
        "async", "await", "try", "catch", "finally", "throw",
        "break", "continue", "switch", "case", "default", "do", "match",
        "import", "from", "null", "true", "false",
        // Type-system keywords previously missing from the dropdown.
        // `annotation` declares user-defined annotation types
        // (`annotation Logged { string level = "info"; }`); `value`
        // marks value classes used by lib/primitives wrappers
        // (`public value class Int { ... }`); `isClassOf` is the
        // runtime type-test operator.
        "annotation", "value", "isClassOf"
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

std::vector<CompletionItem> CompletionHandler::getTypeCompletions(
    const std::string& uri) const
{
    std::vector<CompletionItem> items;

    // Lowercase keywords are true built-ins — never need an import.
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

    // Capitalized wrappers are real classes in lib/primitives/*.mt.
    // We always surface them so the dropdown is predictable, but if
    // the workspace symbol index has the source file AND the wrapper
    // isn't already in scope, attach an auto-import edit so accepting
    // the completion also inserts the matching `import { Int } from
    // "...mt";`. Falls back to a plain item when the workspace can't
    // resolve the wrapper (e.g. lib/primitives lives outside the
    // indexed root).
    static const std::vector<std::string> kWrappers = {
        "Int", "Float", "String", "Bool", "Promise"
    };

    auto doc = documentManager_ ? documentManager_->getDocument(uri) : nullptr;
    auto classRegistry = (doc && doc->environment)
        ? doc->environment->getClassRegistry()
        : nullptr;
    const std::string referencingPath = UriUtils::uriToFilePath(uri);
    const int insertLine = doc ? util::findImportInsertLine(doc->content) : 0;

    for (const auto& wrapper : kWrappers) {
        CompletionItem item;
        item.label = wrapper;
        item.kind = static_cast<int>(CompletionItemKind::Class);
        item.detail = "wrapper type";
        item.insertText = wrapper;
        item.filterText = wrapper;

        // If the wrapper is already imported (visible via the class
        // registry), no auto-import edit needed — just emit plain.
        const bool alreadyInScope =
            classRegistry && classRegistry->hasClass(wrapper);

        if (!alreadyInScope && workspaceIndex_) {
            auto matches = workspaceIndex_->findByName(wrapper, /*maxResults=*/1);
            if (!matches.empty()) {
                const auto& match = matches.front();
                const std::string symbolPath = UriUtils::uriToFilePath(match.fileUri);
                if (symbolPath != referencingPath) {
                    const std::string spelling =
                        analysis::WorkspaceSymbolIndex::computeImportSpelling(
                            symbolPath, referencingPath);
                    item.detail = "Auto-import from \"" + spelling + "\"";
                    item.additionalTextEdits.push_back(
                        utils::buildImportInsertEdit(insertLine, wrapper, spelling));
                }
            }
        }

        items.push_back(std::move(item));
    }

    return items;
}

std::vector<CompletionItem> CompletionHandler::getBuiltinCompletions() const {
    std::vector<CompletionItem> items;

    std::vector<std::tuple<std::string, std::string, std::string>> builtins = {
        {"print", "print(value): void", "print(${1:value})"},
        {"typeof", "typeof(value): string", "typeof(${1:value})"},
        {"hashCode", "hashCode(value): int", "hashCode(${1:value})"},
        {"parsePrimitive", "parsePrimitive(value): string", "parsePrimitive(${1:value})"},
        {"arrayAdd", "arrayAdd(a, b): array", "arrayAdd(${1:a}, ${2:b})"},
        {"arraySum", "arraySum(arr): number", "arraySum(${1:arr})"},
        {"arrayMin", "arrayMin(arr): number", "arrayMin(${1:arr})"},
        {"arrayMax", "arrayMax(arr): number", "arrayMax(${1:arr})"}
    };

    for (const auto& [name, signature, snippet] : builtins) {
        CompletionItem item;
        item.label = name;
        item.kind = static_cast<int>(CompletionItemKind::Function);
        item.detail = signature;
        item.insertText = snippet;
        item.insertTextFormat = kSnippetInsertTextFormat;
        item.filterText = name;
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

std::vector<CompletionItem> CompletionHandler::getIdentifierCompletions(const std::string& uri) const {
    std::vector<CompletionItem> items;

    auto doc = documentManager_->getDocument(uri);
    if (!doc || !doc->environment) {
        return items;
    }

    const auto* env = doc->environment.get();

    // Kind-specialised enumerations. Each of these rides on the
    // same scope-walking + registry-walking logic the diagnostics
    // "did you mean" pool uses (MYT-49 Phase 4), so the completion
    // list is guaranteed to match what the diagnostic system sees.
    auto variables = diagnostics::IdentifierEnumerator::visibleVariables(env);
    auto functions = diagnostics::IdentifierEnumerator::visibleFunctions(env);
    auto classes = diagnostics::IdentifierEnumerator::visibleClasses(env);

    // Track seen names so the same symbol isn't surfaced twice if
    // the enumerator returns overlapping pools (rare, but cheap to
    // guard against).
    std::unordered_set<std::string> seen;

    for (const auto& name : variables) {
        if (!seen.insert(name).second) continue;
        CompletionItem item;
        item.label = name;
        item.kind = static_cast<int>(CompletionItemKind::Variable);
        item.detail = "variable";
        item.insertText = name;
        items.push_back(std::move(item));
    }

    for (const auto& name : functions) {
        if (!seen.insert(name).second) continue;
        CompletionItem item;
        item.label = name;
        item.kind = static_cast<int>(CompletionItemKind::Function);
        item.detail = "function";
        item.insertText = name;
        item.filterText = name;
        if (auto registry = doc->environment->getFunctionRegistry()) {
            if (auto fn = registry->findFunction(name)) {
                std::string returnType = valueTypeToString(fn->getReturnType());
                if (fn->getReturnType() == value::ValueType::OBJECT
                    && !fn->getReturnClassName().empty()) {
                    returnType = fn->getReturnClassName();
                }
                const auto& params = fn->getParametersWithTypes();
                item.detail = name + "("
                    + formatParameters(params)
                    + "): " + returnType;
                // Snippet body so accepting a function completion drops
                // the user inside the call with the first argument
                // selected. handleCompletion strips this back to a bare
                // identifier when the cursor is already followed by `(`.
                item.insertText = buildCallSnippet(name, params);
                item.insertTextFormat = kSnippetInsertTextFormat;
            }
        }
        attachCallCommitChars(item);
        stampResolveData(item, uri, "function", /*owner=*/"", name);
        items.push_back(std::move(item));
    }

    for (const auto& name : classes) {
        if (!seen.insert(name).second) continue;
        CompletionItem item;
        item.label = name;
        item.kind = static_cast<int>(CompletionItemKind::Class);
        item.detail = "class";
        item.insertText = name;
        items.push_back(std::move(item));
    }

    // Interfaces aren't covered by IdentifierEnumerator, so walk the
    // registry directly (same source the old getInterfaceCompletions
    // used). Kept consistent with the other branches for clarity.
    if (auto interfaceRegistry = doc->environment->getInterfaceRegistry()) {
        auto allInterfaces = interfaceRegistry->getAllInterfaces();
        for (const auto& [interfaceName, interfaceDef] : allInterfaces) {
            if (!seen.insert(interfaceName).second) continue;
            CompletionItem item;
            item.label = interfaceName;
            item.kind = static_cast<int>(CompletionItemKind::Interface);
            item.detail = "interface";
            item.insertText = interfaceName;
            items.push_back(std::move(item));
        }
    }

    return items;
}

std::vector<CompletionItem> CompletionHandler::getClassCompletions(const std::string& uri) const {
    std::vector<CompletionItem> items;

    auto doc = documentManager_->getDocument(uri);
    if (!doc || !doc->environment) {
        return items;
    }

    auto classNames =
        diagnostics::IdentifierEnumerator::visibleClasses(doc->environment.get());

    for (const auto& className : classNames) {
        CompletionItem item;
        item.label = className;
        item.kind = static_cast<int>(CompletionItemKind::Class);
        item.detail = "class";
        item.insertText = className;
        items.push_back(std::move(item));
    }

    return items;
}

std::vector<CompletionItem> CompletionHandler::getInterfaceCompletions(const std::string& uri) const {
    std::vector<CompletionItem> items;

    auto doc = documentManager_->getDocument(uri);
    if (!doc || !doc->environment) {
        return items;
    }

    // Route through IdentifierEnumerator so the visibility rules match
    // getClassCompletions / getIdentifierCompletions exactly. The
    // enumerator returns a sorted, deduplicated vector.
    auto interfaceNames =
        diagnostics::IdentifierEnumerator::visibleInterfaces(doc->environment.get());

    for (const auto& interfaceName : interfaceNames) {
        CompletionItem item;
        item.label = interfaceName;
        item.kind = static_cast<int>(CompletionItemKind::Interface);
        item.detail = "interface";
        item.insertText = interfaceName;
        items.push_back(std::move(item));
    }

    return items;
}

std::vector<CompletionItem> CompletionHandler::getAnnotationCompletions(
    const std::string& uri) const
{
    std::vector<CompletionItem> items;

    auto doc = documentManager_->getDocument(uri);
    if (!doc || !doc->environment) {
        return items;
    }

    auto registry = doc->environment->getAnnotationRegistry();
    if (!registry) {
        return items;
    }

    // Built-ins (Override / Script / EntryPoint / Throw / Retention /
    // Target / Inherited) are pre-registered by BuiltInAnnotations at
    // Environment::initialize, so a single registry walk covers both
    // built-ins and any user-declared annotations.
    auto names = registry->getAllItemNames();
    for (const auto& name : names) {
        CompletionItem item;
        item.label = name;
        item.kind = static_cast<int>(CompletionItemKind::Reference);
        item.detail = "annotation";
        item.insertText = name;
        item.filterText = name;
        stampResolveData(item, uri, "annotation", /*owner=*/"", name);
        items.push_back(std::move(item));
    }

    return items;
}

CompletionItem CompletionHandler::resolveCompletion(const CompletionItem& item) const
{
    CompletionItem resolved = item;

    // The resolve round-trip is best-effort: if VS Code dropped the
    // `data` blob, or the document closed since the initial response,
    // we hand the item back unchanged rather than fail the request.
    if (!resolved.data || !resolved.data->is_object()) {
        return resolved;
    }

    const auto& data = *resolved.data;
    if (!data.contains("uri") || !data.contains("kind") || !data.contains("name")) {
        return resolved;
    }

    const std::string uri = data.value("uri", std::string{});
    const std::string kind = data.value("kind", std::string{});
    const std::string owner = data.value("owner", std::string{});
    const std::string name = data.value("name", std::string{});

    auto doc = documentManager_->getDocument(uri);
    if (!doc || !doc->environment) {
        return resolved;
    }

    std::string doc_md;

    if (kind == "function") {
        if (auto registry = doc->environment->getFunctionRegistry()) {
            if (auto fn = registry->findFunction(name)) {
                std::string returnType = valueTypeToString(fn->getReturnType());
                if (fn->getReturnType() == value::ValueType::OBJECT
                    && !fn->getReturnClassName().empty()) {
                    returnType = fn->getReturnClassName();
                }
                doc_md = "**function** `" + name + "("
                    + formatParameters(fn->getParametersWithTypes())
                    + "): " + returnType + "`";
            }
        }
    } else if (kind == "method" || kind == "static_method") {
        if (auto classRegistry = doc->environment->getClassRegistry()) {
            if (auto classDef = classRegistry->findClass(owner)) {
                const auto& methods = (kind == "static_method")
                    ? classDef->getStaticMethods()
                    : classDef->getInstanceMethods();
                auto it = methods.find(name);
                if (it != methods.end() && !it->second.empty()) {
                    doc_md = "**" + std::string(kind == "static_method" ? "static method" : "method")
                        + "** `" + methodDetail(it->second.front(), owner,
                                                it->second.size(),
                                                kind == "static_method") + "`";
                    if (it->second.size() > 1) {
                        doc_md += "\n\nMultiple overloads available.";
                    }
                }
            }
        }
    } else if (kind == "field" || kind == "static_field") {
        if (auto classRegistry = doc->environment->getClassRegistry()) {
            if (auto classDef = classRegistry->findClass(owner)) {
                const auto& fields = (kind == "static_field")
                    ? classDef->getStaticFields()
                    : classDef->getInstanceFields();
                auto it = fields.find(name);
                if (it != fields.end() && it->second) {
                    doc_md = "**" + std::string(kind == "static_field" ? "static field" : "field")
                        + "** `" + fieldDetail(it->second, owner,
                                               kind == "static_field") + "`";
                }
            }
        }
    } else if (kind == "annotation") {
        if (auto registry = doc->environment->getAnnotationRegistry()) {
            if (registry->hasAnnotation(name)) {
                doc_md = "**annotation** `@" + name + "`";
            }
        }
    }

    if (!doc_md.empty()) {
        resolved.documentation = std::move(doc_md);
    }
    return resolved;
}

std::vector<CompletionItem> CompletionHandler::getAutoImportCompletions(
    const std::string& uri,
    const std::string& typedPrefix,
    const std::vector<CompletionItem>& existingItems) const
{
    std::vector<CompletionItem> items;

    if (!workspaceIndex_ || typedPrefix.empty()) {
        return items;
    }

    auto doc = documentManager_->getDocument(uri);
    if (!doc) {
        return items;
    }

    // Short-block until the initial workspace scan is populated. Without
    // this, an early auto-import completion request would see an empty
    // index and offer no suggestions even when the symbol exists.
    workspaceIndex_->waitForReady(std::chrono::milliseconds(50));

    auto matches = workspaceIndex_->findByPrefix(typedPrefix, /*maxResults=*/20);
    if (matches.empty()) {
        return items;
    }

    // Names already surfaced by the in-scope enumeration — skip them
    // so the dropdown doesn't double-list the same symbol.
    std::unordered_set<std::string> existingLabels;
    existingLabels.reserve(existingItems.size());
    for (const auto& item : existingItems) {
        existingLabels.insert(toLowerAscii(item.label));
    }

    const std::string referencingPath = UriUtils::uriToFilePath(uri);
    const int insertLine = util::findImportInsertLine(doc->content);
    std::unordered_set<std::string> emittedAutoImports;

    for (const auto& match : matches) {
        const std::string loweredName = toLowerAscii(match.name);
        if (existingLabels.count(loweredName) != 0) continue;

        const std::string symbolPath = UriUtils::uriToFilePath(match.fileUri);
        if (symbolPath == referencingPath) continue; // same file

        const std::string spelling =
            analysis::WorkspaceSymbolIndex::computeImportSpelling(
                symbolPath, referencingPath);

        const std::string duplicateKey = loweredName + "\n" + spelling;
        if (!emittedAutoImports.insert(duplicateKey).second) continue;

        CompletionItem item;
        item.label = match.name;
        item.kind = static_cast<int>(workspaceKindToCompletionKind(match.kind));
        item.detail = "Auto-import from \"" + spelling + "\"";
        item.insertText = match.name;
        item.filterText = match.name;
        item.additionalTextEdits.push_back(
            utils::buildImportInsertEdit(insertLine, match.name, spelling));
        items.push_back(std::move(item));
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

    // Resolve the type name we're going to walk. For `this` / `super`
    // this depends on the cursor's enclosing class; for a regular
    // identifier we go through DocumentManager's per-document type
    // inference. Failure short-circuits — no point in walking.
    std::string varType;
    if (!isStaticAccess && (objectName == "this" || objectName == "super")) {
        varType = inferEnclosingClass(doc->content, line);
        if (objectName == "super" && !varType.empty()) {
            auto currentClass = classRegistry->findClass(varType);
            std::string parentClass = currentClass ? currentClass->getParentClassName() : "";
            if (parentClass.empty()) {
                parentClass = classRegistry->getParentClass(varType);
            }
            varType = parentClass;
        }
    } else if (isStaticAccess) {
        varType = objectName;
    } else {
        varType = documentManager_->getVariableType(uri, objectName, line);
    }

    if (varType.empty()) {
        return items;
    }
    if (auto angle = varType.find('<'); angle != std::string::npos) {
        varType = varType.substr(0, angle);
    }

    if (!classRegistry->hasClass(varType)) {
        return items;
    }

    // Member completions for `obj.` / `Class::` are pure with respect
    // to (uri, version, varType, accessKind) — the same call returns
    // the same list until the document is edited. The dropdown fires
    // on every keystroke after the trigger, so caching saves the full
    // inheritance walk on each one.
    const std::string cacheKey = uri + "|v" + std::to_string(doc->version)
        + (isStaticAccess ? "|s|" : "|i|") + varType;
    {
        std::lock_guard<std::mutex> lock(memberCacheMutex_);
        auto cacheIt = memberCache_.find(cacheKey);
        if (cacheIt != memberCache_.end()
            && cacheIt->second.docVersion == doc->version) {
            return cacheIt->second.items;
        }
    }

    // Are we inside the class we're querying? Used to decide whether
    // private members are visible. Outside-class access still surfaces
    // protected members so subclass authors can discover them.
    const std::string enclosingClass = inferEnclosingClass(doc->content, line);
    auto isAccessible = [&](ast::AccessModifier modifier,
                            const std::string& memberOwner) {
        if (modifier != ast::AccessModifier::PRIVATE) return true;
        return enclosingClass == memberOwner;
    };

    // Walk the inheritance chain, deduping by (name, isMethod) so an
    // override in the most-derived class wins. Without the walk,
    // members declared on parent classes / Object never showed up
    // after `obj.`.
    std::unordered_set<std::string> seenMembers;
    auto rememberMember = [&](const std::string& name, bool method) {
        return seenMembers.insert((method ? "m:" : "f:") + name).second;
    };

    constexpr int kMaxInheritanceWalk = 32;  // matches ClassDefinition's depth guard
    auto current = classRegistry->findClass(varType);
    int depth = 0;
    while (current && depth++ < kMaxInheritanceWalk) {
        const std::string& currentOwner = current->getName();

        const auto& methods =
            isStaticAccess ? current->getStaticMethods() : current->getInstanceMethods();
        for (const auto& [methodName, overloads] : methods) {
            if (overloads.empty()) continue;
            const auto& firstOverload = overloads.front();
            if (!isAccessible(firstOverload->getAccessModifier(), currentOwner)) continue;
            if (!rememberMember(methodName, /*method=*/true)) continue;

            CompletionItem item;
            item.label = methodName;
            item.kind = static_cast<int>(CompletionItemKind::Method);
            item.detail = methodDetail(firstOverload, currentOwner, overloads.size(), isStaticAccess);
            item.insertText = buildCallSnippet(methodName, firstOverload->getParametersWithTypes());
            item.insertTextFormat = kSnippetInsertTextFormat;
            item.filterText = methodName;
            attachCallCommitChars(item);
            stampResolveData(item, uri,
                             isStaticAccess ? "static_method" : "method",
                             currentOwner, methodName);
            items.push_back(std::move(item));
        }

        const auto& fields =
            isStaticAccess ? current->getStaticFields() : current->getInstanceFields();
        for (const auto& [fieldName, fieldDef] : fields) {
            if (!fieldDef) continue;
            if (!isAccessible(fieldDef->getAccessModifier(), currentOwner)) continue;
            if (!rememberMember(fieldName, /*method=*/false)) continue;

            CompletionItem item;
            item.label = fieldName;
            item.kind = static_cast<int>(CompletionItemKind::Field);
            item.detail = fieldDetail(fieldDef, currentOwner, isStaticAccess);
            item.insertText = fieldName;
            item.filterText = fieldName;
            stampResolveData(item, uri,
                             isStaticAccess ? "static_field" : "field",
                             currentOwner, fieldName);
            items.push_back(std::move(item));
        }

        current = current->getParentClass();
    }

    {
        std::lock_guard<std::mutex> lock(memberCacheMutex_);
        // Evict stale entries for this uri so the cache doesn't grow
        // unbounded across document edits.
        const std::string uriPrefix = uri + "|v";
        for (auto it = memberCache_.begin(); it != memberCache_.end(); ) {
            if (it->first.rfind(uriPrefix, 0) == 0
                && it->second.docVersion != doc->version) {
                it = memberCache_.erase(it);
            } else {
                ++it;
            }
        }
        memberCache_[cacheKey] = MemberCacheEntry{ doc->version, items };
    }

    return items;
}

} // namespace mtype::lsp
