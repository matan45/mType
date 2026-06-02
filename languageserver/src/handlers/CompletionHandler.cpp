#include "CompletionHandler.hpp"

#include "internal/CompletionHelpers.hpp"

#include "../utils/ProjectConfigProvider.hpp"
#include "../analysis/WorkspaceSymbolIndex.hpp"
#include "../../../mType/diagnostics/IdentifierEnumerator.hpp"
#include "../../../mType/environment/Environment.hpp"
#include "../../../mType/environment/registry/AnnotationRegistry.hpp"
#include "../../../mType/environment/registry/FunctionRegistry.hpp"
#include "../../../mType/environment/registry/InterfaceRegistry.hpp"
#include "../../../mType/util/EditDistance.hpp"
#include "../../../mType/util/TokenExtraction.hpp"

#include "../utils/StringUtils.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <optional>
#include <sstream>
#include <utility>

namespace mtype::lsp
{
    namespace
    {
        // Splits the current line into the substring before the cursor
        // and the substring after. Bounds-checks the cursor against the
        // line length so callers can pass either an end-of-line cursor
        // or one past EOL without a substr OOB.
        std::pair<std::string, std::string> splitLineAtCursor(
            const std::string& currentLine, const Position& position)
        {
            const auto len = static_cast<int>(currentLine.length());
            const int col = std::min(position.character, len);
            return {
                currentLine.substr(0, col),
                position.character < len ? currentLine.substr(position.character) : std::string{}
            };
        }

        // Trims trailing whitespace from `text` in place.
        void trimTrailingWhitespace(std::string& text)
        {
            while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
                text.pop_back();
            }
        }

        // Detects the member-access operator (`::` or `.`) at the end of
        // `trimmed`. If the cursor sits mid-identifier just after a
        // member operator (`foo.ba|r`), the typed prefix has eaten the
        // partial token; strip it from `trimmed` so the operator can be
        // re-located. Returns std::nullopt when no operator is found.
        struct MemberOperatorMatch
        {
            std::string trimmed;
            std::size_t operatorPos;
            bool isStaticAccess;
        };

        std::optional<MemberOperatorMatch> findMemberOperator(
            std::string trimmed, const std::string& typedPrefix)
        {
            if (trimmed.length() >= 2 && trimmed.substr(trimmed.length() - 2) == "::") {
                const std::size_t pos = trimmed.length() - 2;
                return MemberOperatorMatch{ std::move(trimmed), pos, true };
            }
            if (!trimmed.empty() && trimmed.back() == '.') {
                const std::size_t pos = trimmed.length() - 1;
                return MemberOperatorMatch{ std::move(trimmed), pos, false };
            }
            if (typedPrefix.empty() || trimmed.length() <= typedPrefix.length()) {
                return std::nullopt;
            }
            std::string beforeTyped =
                trimmed.substr(0, trimmed.length() - typedPrefix.length());
            if (beforeTyped.length() >= 2
                && beforeTyped.substr(beforeTyped.length() - 2) == "::") {
                const std::size_t pos = beforeTyped.length() - 2;
                return MemberOperatorMatch{ std::move(beforeTyped), pos, true };
            }
            if (!beforeTyped.empty() && beforeTyped.back() == '.') {
                const std::size_t pos = beforeTyped.length() - 1;
                return MemberOperatorMatch{ std::move(beforeTyped), pos, false };
            }
            return std::nullopt;
        }

        // Walks backwards from `operatorPos` to find the start of the
        // identifier that owns the member operator.
        std::string identifierBeforeOperator(
            const std::string& trimmed, std::size_t operatorPos)
        {
            std::size_t end = operatorPos;
            // Safe navigation (`foo?.bar`): the member operator is `?.`, so the
            // receiver identifier ends before the `?`, not the `.`.
            if (end > 0 && trimmed[end - 1] == '?') {
                --end;
            }
            std::size_t start = end;
            while (start > 0
                   && (std::isalnum(static_cast<unsigned char>(trimmed[start - 1]))
                       || trimmed[start - 1] == '_')) {
                --start;
            }
            return trimmed.substr(start, end - start);
        }
    }

    CompletionHandler::CompletionHandler(
        DocumentManager* docMgr,
        std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex)
        : documentManager_(docMgr),
          workspaceIndex_(std::move(workspaceIndex)),
          pathCompletionHandler_(std::make_unique<PathCompletionHandler>(docMgr)),
          memberCompletionProvider_(std::make_unique<MemberCompletionProvider>(docMgr)),
          autoImportProvider_(std::make_unique<AutoImportCompletionProvider>(docMgr, workspaceIndex_)),
          completionResolver_(std::make_unique<CompletionResolver>(docMgr)) {}

    void CompletionHandler::setProjectConfig(std::shared_ptr<ProjectConfigProvider> config)
    {
        pathCompletionHandler_->setProjectConfig(std::move(config));
    }

    CompletionItem CompletionHandler::resolveCompletion(const CompletionItem& item) const
    {
        return completionResolver_->resolveCompletion(item);
    }

    std::vector<CompletionItem> CompletionHandler::handleCompletion(
        const std::string& uri, const Position& position)
    {
        auto pathItems = pathCompletionHandler_->getPathCompletions(uri, position);
        if (!pathItems.empty()) return pathItems;

        auto doc = documentManager_->getDocument(uri);
        if (!doc) return {};

        const std::string currentLine = getLineAtPosition(doc->content, position);
        const auto [textBeforeCursor, textAfterCursor] = splitLineAtCursor(currentLine, position);
        const std::string typedPrefix =
            util::extractIdentifierTokenBefore(currentLine, position.character);
        const bool followedByOpenParen = internal::cursorFollowedByOpenParen(textAfterCursor);

        std::string trimmedBefore = textBeforeCursor;
        trimTrailingWhitespace(trimmedBefore);

        if (auto items = tryMemberCompletions(uri, position, trimmedBefore,
                                              typedPrefix, followedByOpenParen)) {
            return std::move(*items);
        }
        if (auto items = tryAnnotationCompletions(uri, position, textBeforeCursor)) {
            return std::move(*items);
        }
        if (auto items = tryInheritanceCompletions(uri, position, textBeforeCursor)) {
            return std::move(*items);
        }
        return buildGeneralCompletions(uri, position, typedPrefix, followedByOpenParen);
    }

    std::optional<std::vector<CompletionItem>> CompletionHandler::tryMemberCompletions(
        const std::string& uri, const Position& position,
        const std::string& trimmedBefore, const std::string& typedPrefix,
        bool followedByOpenParen)
    {
        auto match = findMemberOperator(trimmedBefore, typedPrefix);
        if (!match) return std::nullopt;

        const std::string identifier =
            identifierBeforeOperator(match->trimmed, match->operatorPos);
        if (identifier.empty()) return std::nullopt;

        auto items = memberCompletionProvider_->getMemberCompletions(
            uri, identifier, position.line, match->isStaticAccess);
        applyFuzzyFilter(items, typedPrefix);
        stripCallSnippets(items, followedByOpenParen);
        internal::applyReplacementEdit(items, position, typedPrefix);
        internal::sortCompletions(items, typedPrefix);
        return items;
    }

    std::optional<std::vector<CompletionItem>> CompletionHandler::tryAnnotationCompletions(
        const std::string& uri, const Position& position,
        const std::string& textBeforeCursor) const
    {
        std::string partial;
        if (!internal::textEndsInAnnotationTrigger(textBeforeCursor, partial)) {
            return std::nullopt;
        }
        auto annotations = getAnnotationCompletions(uri);
        applyFuzzyFilter(annotations, partial);
        internal::applyReplacementEdit(annotations, position, partial);
        internal::sortCompletions(annotations, partial);
        return annotations;
    }

    std::optional<std::vector<CompletionItem>> CompletionHandler::tryInheritanceCompletions(
        const std::string& uri, const Position& position,
        const std::string& textBeforeCursor) const
    {
        std::string partial;
        if (internal::textEndsInInheritanceKeyword(textBeforeCursor, "implements", partial)) {
            auto interfaces = getInterfaceCompletions(uri);
            applyFuzzyFilter(interfaces, partial);
            internal::applyReplacementEdit(interfaces, position, partial);
            internal::sortCompletions(interfaces, partial);
            return interfaces;
        }
        if (internal::textEndsInInheritanceKeyword(textBeforeCursor, "extends", partial)) {
            auto classes = getClassCompletions(uri);
            applyFuzzyFilter(classes, partial);
            internal::applyReplacementEdit(classes, position, partial);
            internal::sortCompletions(classes, partial);
            return classes;
        }
        return std::nullopt;
    }

    std::vector<CompletionItem> CompletionHandler::buildGeneralCompletions(
        const std::string& uri, const Position& position,
        const std::string& typedPrefix, bool followedByOpenParen) const
    {
        std::vector<CompletionItem> items;

        auto append = [&items](std::vector<CompletionItem> src) {
            items.insert(items.end(),
                std::make_move_iterator(src.begin()),
                std::make_move_iterator(src.end()));
        };
        append(internal::keywordCompletions());
        append(internal::templateSnippetCompletions());
        append(getTypeCompletions(uri));
        append(internal::builtinCompletions());
        append(internal::collectionCompletions());
        append(getIdentifierCompletions(uri));

        // Filter the in-scope pool before adding auto-import items so
        // workspace matches aren't fighting in-scope matches for the
        // same typed prefix.
        applyFuzzyFilter(items, typedPrefix);

        auto autoImports = autoImportProvider_->getAutoImportCompletions(uri, typedPrefix, items);
        items.insert(items.end(), autoImports.begin(), autoImports.end());

        stripCallSnippets(items, followedByOpenParen);
        internal::applyReplacementEdit(items, position, typedPrefix);
        internal::sortCompletions(items, typedPrefix);
        return items;
    }

    std::string CompletionHandler::getLineAtPosition(
        const std::string& content, const Position& position) const
    {
        std::istringstream stream(content);
        std::string line;
        int currentLine = 0;
        while (std::getline(stream, line) && currentLine < position.line) {
            ++currentLine;
        }
        return currentLine == position.line ? line : std::string{};
    }

    void CompletionHandler::applyFuzzyFilter(
        std::vector<CompletionItem>& items, const std::string& typedPrefix) const
    {
        if (typedPrefix.empty()) return;

        const int budget = std::max(1, static_cast<int>(typedPrefix.size()) / 3);
        const std::string loweredPrefix = utils::toLowerAscii(typedPrefix);
        items.erase(std::remove_if(items.begin(), items.end(),
            [&loweredPrefix, budget](const CompletionItem& item) {
                const std::string loweredLabel = utils::toLowerAscii(item.label);
                // Keep prefix matches unconditionally — VS Code will
                // re-score them and put them at the top of the list.
                if (loweredLabel.rfind(loweredPrefix, 0) == 0) return false;
                return util::levenshtein(loweredPrefix, loweredLabel, budget) > budget;
            }),
            items.end());
    }

    void CompletionHandler::stripCallSnippets(
        std::vector<CompletionItem>& items, bool followedByOpenParen) const
    {
        if (!followedByOpenParen) return;
        for (auto& item : items) {
            if (!internal::isCallableKind(item.kind)) continue;
            if (item.insertTextFormat.value_or(1) != internal::kSnippetInsertTextFormat) continue;
            item.insertText = item.label;
            item.insertTextFormat.reset();
        }
    }

    std::vector<CompletionItem> CompletionHandler::getTypeCompletions(
        const std::string& uri) const
    {
        std::vector<CompletionItem> items;

        // Lowercase keywords are true built-ins — never need an import.
        static const std::vector<std::string> kPrimitives = {
            "int", "float", "string", "bool", "void"
        };
        for (const auto& type : kPrimitives) {
            CompletionItem item;
            item.label = type;
            item.kind = static_cast<int>(CompletionItemKind::Class);
            item.detail = "primitive type";
            item.insertText = type;
            items.push_back(std::move(item));
        }

        // Capitalized wrappers are real classes in lib/primitives/*.mt.
        // Surface them unconditionally; AutoImportCompletionProvider
        // attaches an Auto-import edit when the wrapper isn't already
        // in scope and the workspace index can resolve it.
        static const std::vector<std::string> kWrappers = {
            "Int", "Float", "String", "Bool", "Promise"
        };
        for (const auto& wrapper : kWrappers) {
            CompletionItem item;
            item.label = wrapper;
            item.kind = static_cast<int>(CompletionItemKind::Class);
            item.detail = "wrapper type";
            item.insertText = wrapper;
            item.filterText = wrapper;
            autoImportProvider_->enrichWithWrapperImport(item, wrapper, uri);
            items.push_back(std::move(item));
        }
        return items;
    }

    std::vector<CompletionItem> CompletionHandler::getAnnotationCompletions(
        const std::string& uri) const
    {
        std::vector<CompletionItem> items;
        auto doc = documentManager_->getDocument(uri);
        if (!doc || !doc->environment) return items;

        auto registry = doc->environment->getAnnotationRegistry();
        if (!registry) return items;

        // Built-ins (Override / Script / EntryPoint / Throw / Retention /
        // Target / Inherited) are pre-registered by BuiltInAnnotations at
        // Environment::initialize, so a single registry walk covers both
        // built-ins and any user-declared annotations.
        const auto names = registry->getAllItemNames();
        items.reserve(names.size());
        for (const auto& name : names) {
            CompletionItem item;
            item.label = name;
            item.kind = static_cast<int>(CompletionItemKind::Reference);
            item.detail = "annotation";
            item.insertText = name;
            item.filterText = name;
            internal::stampResolveData(item, uri, "annotation", /*owner=*/"", name);
            items.push_back(std::move(item));
        }
        return items;
    }

    std::vector<CompletionItem> CompletionHandler::getIdentifierCompletions(
        const std::string& uri) const
    {
        std::vector<CompletionItem> items;
        auto doc = documentManager_->getDocument(uri);
        if (!doc || !doc->environment) return items;

        const auto& env = *doc->environment;
        // Shared dedup set so the same name isn't surfaced twice across
        // overlapping pools (variable + class with same casing is rare,
        // but the guard is cheap).
        std::unordered_set<std::string> seen;
        addVariableItems(items, env, seen);
        addFunctionItems(items, env, uri, seen);
        addClassItems(items, env, seen);
        addInterfaceItems(items, env, seen);
        return items;
    }

    void CompletionHandler::addVariableItems(
        std::vector<CompletionItem>& items,
        const environment::Environment& env,
        std::unordered_set<std::string>& seen) const
    {
        auto variables = diagnostics::IdentifierEnumerator::visibleVariables(&env);
        for (const auto& name : variables) {
            if (!seen.insert(name).second) continue;
            CompletionItem item;
            item.label = name;
            item.kind = static_cast<int>(CompletionItemKind::Variable);
            item.detail = "variable";
            item.insertText = name;
            items.push_back(std::move(item));
        }
    }

    void CompletionHandler::addFunctionItems(
        std::vector<CompletionItem>& items,
        const environment::Environment& env,
        const std::string& uri,
        std::unordered_set<std::string>& seen) const
    {
        auto functions = diagnostics::IdentifierEnumerator::visibleFunctions(&env);
        auto registry = env.getFunctionRegistry();
        for (const auto& name : functions) {
            if (!seen.insert(name).second) continue;
            CompletionItem item;
            item.label = name;
            item.kind = static_cast<int>(CompletionItemKind::Function);
            item.detail = "function";
            item.insertText = name;
            item.filterText = name;
            if (registry) {
                if (auto fn = registry->findFunction(name)) {
                    std::string returnType = internal::valueTypeToString(fn->getReturnType());
                    if (fn->getReturnType() == value::ValueType::OBJECT
                        && !fn->getReturnClassName().empty()) {
                        returnType = fn->getReturnClassName();
                    }
                    const auto& params = fn->getParametersWithTypes();
                    item.detail = name + "(" + internal::formatParameters(params)
                        + "): " + returnType;
                    item.insertText = internal::buildCallSnippet(name, params);
                    item.insertTextFormat = internal::kSnippetInsertTextFormat;
                }
            }
            internal::attachCallCommitChars(item);
            internal::stampResolveData(item, uri, "function", /*owner=*/"", name);
            items.push_back(std::move(item));
        }
    }

    void CompletionHandler::addClassItems(
        std::vector<CompletionItem>& items,
        const environment::Environment& env,
        std::unordered_set<std::string>& seen) const
    {
        auto classes = diagnostics::IdentifierEnumerator::visibleClasses(&env);
        for (const auto& name : classes) {
            if (!seen.insert(name).second) continue;
            CompletionItem item;
            item.label = name;
            item.kind = static_cast<int>(CompletionItemKind::Class);
            item.detail = "class";
            item.insertText = name;
            items.push_back(std::move(item));
        }
    }

    void CompletionHandler::addInterfaceItems(
        std::vector<CompletionItem>& items,
        const environment::Environment& env,
        std::unordered_set<std::string>& seen) const
    {
        // Interfaces aren't covered by IdentifierEnumerator; walk the
        // registry directly. Same source the standalone
        // getInterfaceCompletions uses.
        auto interfaceRegistry = env.getInterfaceRegistry();
        if (!interfaceRegistry) return;
        const auto& allInterfaces = interfaceRegistry->getAllInterfaces();
        for (const auto& [name, def] : allInterfaces) {
            if (!seen.insert(name).second) continue;
            CompletionItem item;
            item.label = name;
            item.kind = static_cast<int>(CompletionItemKind::Interface);
            item.detail = "interface";
            item.insertText = name;
            items.push_back(std::move(item));
        }
    }

    std::vector<CompletionItem> CompletionHandler::getClassCompletions(
        const std::string& uri) const
    {
        std::vector<CompletionItem> items;
        auto doc = documentManager_->getDocument(uri);
        if (!doc || !doc->environment) return items;

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

    std::vector<CompletionItem> CompletionHandler::getInterfaceCompletions(
        const std::string& uri) const
    {
        std::vector<CompletionItem> items;
        auto doc = documentManager_->getDocument(uri);
        if (!doc || !doc->environment) return items;

        // Route through IdentifierEnumerator so the visibility rules match
        // getClassCompletions / getIdentifierCompletions exactly.
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
}
