#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../../utils/LSPTypes.hpp"
#include "../../analysis/WorkspaceSymbolIndex.hpp"
#include "../../../../mType/value/ParameterType.hpp"

namespace runtimeTypes::klass
{
    class MethodDefinition;
    class FieldDefinition;
}

namespace mtype::lsp::internal
{
    constexpr int kSnippetInsertTextFormat = 2;

    // Priority + sort helpers.
    int kindPriority(int kind);
    int matchPriority(const CompletionItem& item, const std::string& typedPrefix);
    bool isAutoImportItem(const CompletionItem& item);
    int completionPriority(const CompletionItem& item, const std::string& typedPrefix);
    void applySortText(std::vector<CompletionItem>& items, const std::string& typedPrefix);
    void sortCompletions(std::vector<CompletionItem>& items, const std::string& typedPrefix);
    void applyReplacementEdit(std::vector<CompletionItem>& items,
                              const Position& position,
                              const std::string& typedPrefix);

    // Completion-item formatting.
    CompletionItemKind workspaceKindToCompletionKind(analysis::WorkspaceSymbolKind kind);
    std::string formatParameters(
        const std::vector<std::pair<std::string, value::ParameterType>>& params);
    std::string valueTypeToString(value::ValueType type);
    std::string methodDetail(
        const std::shared_ptr<runtimeTypes::klass::MethodDefinition>& method,
        const std::string& owner,
        std::size_t overloadCount,
        bool isStatic);
    std::string fieldDetail(
        const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field,
        const std::string& owner,
        bool isStatic);

    // Snippet + item-metadata helpers.
    std::string buildCallSnippet(
        const std::string& name,
        const std::vector<std::pair<std::string, value::ParameterType>>& params);
    bool cursorFollowedByOpenParen(const std::string& textAfterCursor);
    void stampResolveData(CompletionItem& item,
                          const std::string& uri,
                          const std::string& kind,
                          const std::string& owner,
                          const std::string& name);
    void attachCallCommitChars(CompletionItem& item);
    bool isCallableKind(int kind);

    // Context detection. `inferEnclosingClass` doesn't recognise braces
    // inside string literals or comments — the brace-counter is a text
    // scan, not a lexer pass. Documented caveat at the implementation.
    bool textEndsInAnnotationTrigger(const std::string& text, std::string& outPartial);
    bool textEndsInInheritanceKeyword(const std::string& text,
                                      const char* keyword,
                                      std::string& outPartial);
    std::string inferEnclosingClass(const std::string& content, int targetLine);

    // Stateless completion catalogues. These don't depend on the document
    // environment, so they live alongside the other helpers rather than
    // as CompletionHandler members.
    std::vector<CompletionItem> keywordCompletions();
    std::vector<CompletionItem> templateSnippetCompletions();
    std::vector<CompletionItem> builtinCompletions();
    std::vector<CompletionItem> collectionCompletions();
}
