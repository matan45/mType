#pragma once

#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "DocumentManager.hpp"
#include "handlers/CompletionHandler.hpp"
#include "handlers/DiagnosticsHandler.hpp"
#include "handlers/HoverHandler.hpp"
#include "handlers/DefinitionHandler.hpp"
#include "handlers/CodeActionHandler.hpp"
#include "handlers/CodeLensHandler.hpp"
#include "handlers/FormattingHandler.hpp"
#include "handlers/ReferencesHandler.hpp"
#include "handlers/SignatureHelpHandler.hpp"
#include "handlers/SemanticTokensHandler.hpp"
#include "analysis/WorkspaceSymbolIndex.hpp"
#include "utils/JsonRpc.hpp"
#include "utils/LSPTypes.hpp"
#include "utils/ProjectConfigProvider.hpp"

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
    void handleReferences(const json& id, const json& params);
    void handleSignatureHelp(const json& id, const json& params);
    void handleSemanticTokensFull(const json& id, const json& params);

    // Utility methods
    void sendResponse(const json& id, const json& result);
    void sendError(const json& id, int code, const std::string& message);
    void publishDiagnostics(const std::string& uri, const std::vector<Diagnostic>& diagnostics);
    void logMessage(const std::string& message);

    // Components
    std::unique_ptr<DocumentManager> documentManager_;
    std::unique_ptr<CompletionHandler> completionHandler_;
    std::unique_ptr<DiagnosticsHandler> diagnosticsHandler_;
    std::unique_ptr<HoverHandler> hoverHandler_;
    std::unique_ptr<DefinitionHandler> definitionHandler_;
    std::unique_ptr<CodeActionHandler> codeActionHandler_;
    std::unique_ptr<CodeLensHandler> codeLensHandler_;
    std::unique_ptr<FormattingHandler> formattingHandler_;
    std::unique_ptr<ReferencesHandler> referencesHandler_;
    std::unique_ptr<SignatureHelpHandler> signatureHelpHandler_;
    std::unique_ptr<SemanticTokensHandler> semanticTokensHandler_;
    std::shared_ptr<ProjectConfigProvider> projectConfig_;

    // MYT-47 — workspace-wide symbol index used by the missing-import
    // quick fix. Built off-thread at initialise so a large workspace
    // doesn't block the LSP handshake.
    std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex_;
    std::shared_future<void> workspaceIndexReady_;

    // Debouncer for per-keystroke reindex requests. The reindex itself is
    // moderately expensive (full lex+parse), and VS Code sends one change
    // event per keystroke for full-document sync. The debouncer holds a
    // monotonic version per URI; each scheduled task wakes after the idle
    // window and only runs reindex if its captured version is still the
    // latest. Detached so it doesn't block message processing; held by
    // shared_ptr so in-flight tasks survive LSP destruction without
    // touching `this`.
    struct ReindexDebouncer
    {
        std::mutex mutex;
        std::unordered_map<std::string, std::uint64_t> versions;
    };
    std::shared_ptr<ReindexDebouncer> reindexDebouncer_;

    void scheduleDebouncedReindex(const std::string& uri, const std::string& content);

    bool shouldExit_ = false;
};

} // namespace mtype::lsp
