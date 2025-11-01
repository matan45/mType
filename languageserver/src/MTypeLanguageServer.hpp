#pragma once

#include <memory>
#include "DocumentManager.hpp"
#include "handlers/CompletionHandler.hpp"
#include "handlers/DiagnosticsHandler.hpp"
#include "handlers/HoverHandler.hpp"
#include "handlers/DefinitionHandler.hpp"
#include "handlers/CodeActionHandler.hpp"
#include "handlers/CodeLensHandler.hpp"
#include "handlers/FormattingHandler.hpp"
#include "utils/JsonRpc.hpp"
#include "utils/LSPTypes.hpp"

namespace mtype::lsp {

class MTypeLanguageServer {
public:
    MTypeLanguageServer();
    ~MTypeLanguageServer();

    void run();

private:
    void handleMessage(const json& message);
    void handleRequest(const json& id, const std::string& method, const json& params);
    void handleNotification(const std::string& method, const json& params);

    // LSP method handlers
    void handleInitialize(const json& id, const json& params);
    void handleShutdown(const json& id);
    void handleDidOpenTextDocument(const json& params);
    void handleDidChangeTextDocument(const json& params);
    void handleDidCloseTextDocument(const json& params);
    void handleCompletion(const json& id, const json& params);
    void handleHover(const json& id, const json& params);
    void handleDefinition(const json& id, const json& params);
    void handleCodeAction(const json& id, const json& params);
    void handleCodeLens(const json& id, const json& params);
    void handleFormatting(const json& id, const json& params);

    // Utility methods
    void sendResponse(const json& id, const json& result);
    void sendError(const json& id, int code, const std::string& message);
    void publishDiagnostics(const std::string& uri, const std::vector<Diagnostic>& diagnostics);

    // Components
    std::unique_ptr<DocumentManager> documentManager_;
    std::unique_ptr<CompletionHandler> completionHandler_;
    std::unique_ptr<DiagnosticsHandler> diagnosticsHandler_;
    std::unique_ptr<HoverHandler> hoverHandler_;
    std::unique_ptr<DefinitionHandler> definitionHandler_;
    std::unique_ptr<CodeActionHandler> codeActionHandler_;
    std::unique_ptr<CodeLensHandler> codeLensHandler_;
    std::unique_ptr<FormattingHandler> formattingHandler_;

    bool shouldExit_ = false;
};

} // namespace mtype::lsp
