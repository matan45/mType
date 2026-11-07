#include "CodeActionMerger.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>

namespace mtype::lsp {

namespace {

std::string keyOf(const CodeAction& action) {
    std::string key = action.title;
    key += '\0';
    key += action.kind.value_or("");
    key += '\0';
    if (action.edit) key += action.edit->toJson().dump();
    return key;
}

void addMissingDiagnostics(CodeAction& target, const std::vector<Diagnostic>& extra) {
    for (const auto& diagnostic : extra) {
        const json wanted = diagnostic.toJson();
        const bool present = std::any_of(target.diagnostics.begin(), target.diagnostics.end(),
            [&wanted](const Diagnostic& existing) { return existing.toJson() == wanted; });
        if (!present) target.diagnostics.push_back(diagnostic);
    }
}

} // namespace

std::vector<CodeAction> CodeActionMerger::mergeDuplicates(std::vector<CodeAction> actions) {
    std::vector<CodeAction> merged;
    merged.reserve(actions.size());
    std::unordered_map<std::string, std::size_t> indexByKey;
    for (auto& action : actions) {
        const auto [it, inserted] = indexByKey.emplace(keyOf(action), merged.size());
        if (inserted) {
            merged.push_back(std::move(action));
        } else {
            addMissingDiagnostics(merged[it->second], action.diagnostics);
        }
    }
    return merged;
}

} // namespace mtype::lsp
