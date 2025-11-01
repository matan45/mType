#include "DiagnosticsHandler.hpp"

namespace mtype::lsp {

DiagnosticsHandler::DiagnosticsHandler(DocumentManager* docMgr)
    : documentManager_(docMgr) {}

void DiagnosticsHandler::setPublisher(DiagnosticPublisher publisher) {
    publisher_ = std::move(publisher);
}

void DiagnosticsHandler::publishDiagnostics(const std::string& uri) {
    auto doc = documentManager_->getDocument(uri);
    if (!doc) return;

    auto diagnostics = analyzeDiagnostics(doc);

    if (publisher_) {
        publisher_(uri, diagnostics);
    }
}

std::vector<Diagnostic> DiagnosticsHandler::analyzeDiagnostics(const Document* doc) {
    std::vector<Diagnostic> diagnostics;

    if (!doc) return diagnostics;

    // Add parse errors
    for (const auto& error : doc->parseErrors) {
        Diagnostic diag;
        diag.range = Range{{0, 0}, {0, 0}};  // Default range
        diag.severity = static_cast<int>(DiagnosticSeverity::Error);
        diag.message = "Parse error: " + error;
        diag.source = "mType Parser";
        diagnostics.push_back(diag);
    }

    // Add semantic errors
    for (const auto& error : doc->semanticErrors) {
        Diagnostic diag;
        diag.range = Range{{0, 0}, {0, 0}};  // Default range
        diag.severity = static_cast<int>(DiagnosticSeverity::Error);
        diag.message = "Semantic error: " + error;
        diag.source = "mType Analyzer";
        diagnostics.push_back(diag);
    }

    // TODO: Add more sophisticated error checking with proper source locations
    // - Type errors with specific line/column
    // - Undefined variables
    // - Unused variables (warning)
    // - etc.

    return diagnostics;
}

} // namespace mtype::lsp
