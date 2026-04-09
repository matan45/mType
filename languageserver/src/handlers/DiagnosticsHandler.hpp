#pragma once

#include <vector>
#include <functional>
#include <memory>
#include "../utils/LSPTypes.hpp"
#include "../utils/ProjectConfigProvider.hpp"
#include "../DocumentManager.hpp"

namespace mtype::lsp {

class DiagnosticsHandler {
public:
    using DiagnosticPublisher = std::function<void(const std::string& uri, const std::vector<Diagnostic>&)>;

    explicit DiagnosticsHandler(DocumentManager* docMgr);

    void setPublisher(DiagnosticPublisher publisher);
    void setProjectConfig(std::shared_ptr<ProjectConfigProvider> config);
    void publishDiagnostics(const std::string& uri);

private:
    std::vector<Diagnostic> analyzeDiagnostics(const Document* doc);
    std::vector<Diagnostic> validateImportPaths(const Document* doc);
    std::string resolveImportPath(const std::string& baseUri, const std::string& relativePath);

    DocumentManager* documentManager_;
    DiagnosticPublisher publisher_;
    std::shared_ptr<ProjectConfigProvider> projectConfig_;
};

} // namespace mtype::lsp
