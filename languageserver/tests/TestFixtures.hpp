#pragma once

#include "../src/DocumentManager.hpp"
#include "../src/analysis/WorkspaceSymbolIndex.hpp"
#include "../src/utils/LSPTypes.hpp"

#include <memory>
#include <string>

namespace mtype::lsp::test {

// Creates a DocumentManager with a single document opened and parsed.
// Returns a unique_ptr because DocumentManager is non-copyable (owns unique_ptr<ImportResolver>).
inline std::unique_ptr<DocumentManager> makeDocManager(const std::string& uri, const std::string& source) {
    auto docMgr = std::make_unique<DocumentManager>();
    docMgr->openDocument(uri, source, 1);
    docMgr->parseDocument(uri);
    return docMgr;
}

// Builds an LSP Diagnostic with a data blob containing the exceptionType.
inline Diagnostic makeDiagnostic(const Range& range,
                                  const std::string& message,
                                  const std::string& exceptionType = "") {
    Diagnostic diag;
    diag.range = range;
    diag.severity = 1;
    diag.message = message;
    if (!exceptionType.empty()) {
        diag.data = nlohmann::json{{"exceptionType", exceptionType}};
    }
    return diag;
}

// Builds an LSP Diagnostic with a suggestions data blob for typo fixes.
inline Diagnostic makeDiagnosticWithSuggestions(
    const Range& range,
    const std::string& message,
    const std::vector<std::string>& suggestions) {
    Diagnostic diag;
    diag.range = range;
    diag.severity = 1;
    diag.message = message;
    nlohmann::json sugArr = nlohmann::json::array();
    for (const auto& s : suggestions) {
        sugArr.push_back(nlohmann::json{{"label", "did you mean '" + s + "'?"}});
    }
    diag.data = nlohmann::json{{"suggestions", sugArr}};
    return diag;
}

} // namespace mtype::lsp::test
