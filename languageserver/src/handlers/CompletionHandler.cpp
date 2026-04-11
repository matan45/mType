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
#include <sstream>
#include <unordered_set>

namespace mtype::lsp {

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

    // MYT-51 — extract the partial identifier under the cursor once,
    // upfront, so every branch can apply the same fuzzy filter.
    const std::string typedPrefix =
        util::extractIdentifierTokenBefore(currentLine, position.character);

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
            return memberItems;
        }
    }

    // Check context-specific completions
    // If we're after "implements", only show interfaces
    if (textBeforeCursor.find("implements") != std::string::npos) {
        auto interfaces = getInterfaceCompletions(uri);
        applyFuzzyFilter(interfaces, typedPrefix);
        return interfaces;
    }

    // If we're after "extends", only show classes
    if (textBeforeCursor.find("extends") != std::string::npos) {
        auto classes = getClassCompletions(uri);
        applyFuzzyFilter(classes, typedPrefix);
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

    items.erase(std::remove_if(items.begin(), items.end(),
        [&typedPrefix, budget](const CompletionItem& item) {
            // Keep prefix matches unconditionally — VS Code will
            // re-score them and put them at the top of the list.
            if (item.label.rfind(typedPrefix, 0) == 0) return false;
            // Otherwise drop anything outside the rustc-style budget.
            return util::levenshtein(typedPrefix, item.label, budget) > budget;
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

    auto interfaceRegistry = doc->environment->getInterfaceRegistry();
    if (!interfaceRegistry) {
        return items;
    }

    auto allInterfaces = interfaceRegistry->getAllInterfaces();
    for (const auto& [interfaceName, interfaceDef] : allInterfaces) {
        CompletionItem item;
        item.label = interfaceName;
        item.kind = static_cast<int>(CompletionItemKind::Interface);
        item.detail = "interface";
        item.insertText = interfaceName;
        items.push_back(std::move(item));
    }

    return items;
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

    auto matches = workspaceIndex_->findByName(typedPrefix, /*maxResults=*/5);
    if (matches.empty()) {
        return items;
    }

    // Names already surfaced by the in-scope enumeration — skip them
    // so the dropdown doesn't double-list the same symbol.
    std::unordered_set<std::string> existingLabels;
    existingLabels.reserve(existingItems.size());
    for (const auto& item : existingItems) {
        existingLabels.insert(item.label);
    }

    const std::string referencingPath = UriUtils::uriToFilePath(uri);
    const int insertLine = util::findImportInsertLine(doc->content);

    for (const auto& match : matches) {
        if (existingLabels.count(match.name) != 0) continue;

        const std::string symbolPath = UriUtils::uriToFilePath(match.fileUri);
        if (symbolPath == referencingPath) continue; // same file

        const std::string spelling =
            analysis::WorkspaceSymbolIndex::computeImportSpelling(
                symbolPath, referencingPath);

        CompletionItem item;
        item.label = match.name;
        item.kind = static_cast<int>(CompletionItemKind::Class);
        item.detail = "Auto-import from \"" + spelling + "\"";
        item.insertText = match.name;
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
