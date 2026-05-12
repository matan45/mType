#include "MTypeLanguageServer.hpp"
#include "utils/UriUtils.hpp"
#include "version/Version.hpp"
#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <thread>
#include <typeinfo>

namespace mtype::lsp
{
    namespace
    {
        // Idle window for debouncing per-keystroke reindex requests. Tuned
        // to absorb a fast typing burst (~5 keys/sec) without blocking the
        // moment the user pauses. Tweak if it feels sluggish.
        constexpr auto kReindexDebounceWindow = std::chrono::milliseconds(250);
    }

    MTypeLanguageServer::MTypeLanguageServer()
    {
        documentManager_ = std::make_unique<DocumentManager>();
        // MYT-47 — workspace symbol index, populated lazily during
        // handleInitialize once we know the workspace root.
        workspaceIndex_ = std::make_shared<analysis::WorkspaceSymbolIndex>();
        reindexDebouncer_ = std::make_shared<ReindexDebouncer>();

        // MYT-51 — the completion handler shares the same workspace
        // symbol index as the code-action handler so the auto-import
        // completion branch and the missing-import quick fix see the
        // same symbol pool.
        completionHandler_ = std::make_unique<CompletionHandler>(
            documentManager_.get(), workspaceIndex_);
        diagnosticsHandler_ = std::make_unique<DiagnosticsHandler>(documentManager_.get());
        hoverHandler_ = std::make_unique<HoverHandler>(documentManager_.get());
        definitionHandler_ = std::make_unique<DefinitionHandler>(documentManager_.get());
        codeActionHandler_ = std::make_unique<CodeActionHandler>(
            documentManager_.get(), workspaceIndex_);
        codeLensHandler_ = std::make_unique<CodeLensHandler>(documentManager_.get());
        formattingHandler_ = std::make_unique<FormattingHandler>();
        referencesHandler_ = std::make_unique<ReferencesHandler>(
            documentManager_.get(), workspaceIndex_);
        // MYT-294 — rename walks every open document via documentManager_;
        // it does not consult the workspace index because the index only
        // tracks files that *declare* top-level symbols, missing files
        // that only *use* the symbol being renamed.
        renameHandler_ = std::make_unique<RenameHandler>(
            documentManager_.get());
        signatureHelpHandler_ = std::make_unique<SignatureHelpHandler>(documentManager_.get());
        semanticTokensHandler_ = std::make_unique<SemanticTokensHandler>(documentManager_.get());
        inlayHintHandler_ = std::make_unique<InlayHintHandler>(documentManager_.get());
        documentSymbolHandler_ = std::make_unique<DocumentSymbolHandler>(documentManager_.get());
        // MYT-297 — workspace/symbol shares the same workspace index
        // populated at initialise so cross-file symbol search sees the
        // same pool as completion, references, and rename.
        workspaceSymbolHandler_ = std::make_unique<WorkspaceSymbolHandler>(workspaceIndex_);

        // Set up diagnostics publisher
        diagnosticsHandler_->setPublisher(
            [this](const std::string& uri, const std::vector<Diagnostic>& diags)
            {
                publishDiagnostics(uri, diags);
            }
        );
    }

    MTypeLanguageServer::~MTypeLanguageServer() = default;

    void MTypeLanguageServer::run()
    {
        while (!shouldExit_)
        {
            try
            {
                std::string content = JsonRpc::readMessage(std::cin);
                if (content.empty())
                {
                    break;
                }

                json message = json::parse(content);
                handleMessage(message);
            }
            catch (const json::parse_error& e)
            {
                // Malformed frame from the client — log and keep reading. The
                // LSP protocol allows free-form stderr for server diagnostics;
                // VS Code surfaces this in the "MType" output channel.
                std::cerr << "[mtype-lsp] json parse error: " << e.what() << "\n";
            }
            catch (const std::exception& e)
            {
                // Anything else — bad_alloc, json::type_error from a malformed
                // request, runtime_error from a handler — used to be swallowed
                // silently, turning unrecoverable errors into an invisible
                // busy loop. Surface them so failures are at least diagnosable.
                std::cerr << "[mtype-lsp] " << typeid(e).name() << ": " << e.what() << "\n";
            }
        }
    }

    void MTypeLanguageServer::handleMessage(const json& message)
    {
        if (!message.contains("method"))
        {
            return; // Response message, ignore for now
        }

        // The implicit string conversion below throws json::type_error if
        // "method" is anything other than a string. Validate explicitly so a
        // malformed request becomes a JSON-RPC -32600 (Invalid Request) reply
        // rather than relying on the run() loop's catch-all to silently
        // discard the frame.
        if (!message["method"].is_string())
        {
            if (message.contains("id"))
            {
                sendError(message["id"], -32600, "method must be a string");
            }
            return;
        }

        std::string method = message["method"];
        json params = message.contains("params") ? message["params"] : json::object();

        if (message.contains("id"))
        {
            // Request
            handleRequest(message["id"], method, params);
        }
        else
        {
            // Notification
            handleNotification(method, params);
        }
    }

    void MTypeLanguageServer::handleRequest(const json& id, const std::string& method, const json& params)
    {
        if (method == "initialize")
        {
            handleInitialize(id, params);
        }
        else if (method == "shutdown")
        {
            handleShutdown(id);
        }
        else if (method == "textDocument/completion")
        {
            handleCompletion(id, params);
        }
        else if (method == "completionItem/resolve")
        {
            handleCompletionItemResolve(id, params);
        }
        else if (method == "textDocument/hover")
        {
            handleHover(id, params);
        }
        else if (method == "textDocument/definition")
        {
            handleDefinition(id, params);
        }
        else if (method == "textDocument/codeAction")
        {
            handleCodeAction(id, params);
        }
        else if (method == "textDocument/codeLens")
        {
            handleCodeLens(id, params);
        }
        else if (method == "textDocument/formatting")
        {
            handleFormatting(id, params);
        }
        else if (method == "textDocument/references")
        {
            handleReferences(id, params);
        }
        else if (method == "textDocument/prepareRename")
        {
            handlePrepareRename(id, params);
        }
        else if (method == "textDocument/rename")
        {
            handleRename(id, params);
        }
        else if (method == "textDocument/signatureHelp")
        {
            handleSignatureHelp(id, params);
        }
        else if (method == "textDocument/semanticTokens/full")
        {
            handleSemanticTokensFull(id, params);
        }
        else if (method == "textDocument/inlayHint")
        {
            handleInlayHint(id, params);
        }
        else if (method == "textDocument/documentSymbol")
        {
            handleDocumentSymbol(id, params);
        }
        else if (method == "workspace/symbol")
        {
            handleWorkspaceSymbol(id, params);
        }
        else
        {
            sendError(id, -32601, "Method not found: " + method);
        }
    }

    void MTypeLanguageServer::handleNotification(const std::string& method, const json& params)
    {
        if (method == "initialized")
        {
            // Client initialized, nothing to do
        }
        else if (method == "exit")
        {
            shouldExit_ = true;
        }
        else if (method == "textDocument/didOpen")
        {
            handleDidOpenTextDocument(params);
        }
        else if (method == "textDocument/didChange")
        {
            handleDidChangeTextDocument(params);
        }
        else if (method == "textDocument/didClose")
        {
            handleDidCloseTextDocument(params);
        }
    }

    void MTypeLanguageServer::handleInitialize(const json& id, const json& params)
    {
        // Extract workspace root from initialize params
        projectConfig_ = std::make_shared<ProjectConfigProvider>();
        std::string workspaceRoot;

        if (params.contains("rootUri") && params["rootUri"].is_string())
        {
            workspaceRoot = UriUtils::uriToFilePath(params["rootUri"]);
        }
        else if (params.contains("rootPath") && params["rootPath"].is_string())
        {
            workspaceRoot = params["rootPath"];
        }

        if (!workspaceRoot.empty())
        {
            projectConfig_->loadFromWorkspace(workspaceRoot);

            // MYT-47 — kick off the initial workspace symbol scan in a
            // background thread. The future is shared with the index so
            // request handlers (the missing-import quick fix and the
            // auto-import completion branch) can short-block early
            // requests via WorkspaceSymbolIndex::waitForReady().
            auto idx = workspaceIndex_;
            std::string root = workspaceRoot;
            workspaceIndexReady_ = std::async(std::launch::async,
                [idx, root]() {
                    idx->buildFromWorkspace(root);
                }).share();
            workspaceIndex_->setReadyFuture(workspaceIndexReady_);
        }

        // Share project config with handlers
        diagnosticsHandler_->setProjectConfig(projectConfig_);
        documentManager_->setProjectConfig(projectConfig_);

        json capabilities = {
            {"textDocumentSync", 1}, // Full sync
            {
                "completionProvider", {
                    // Lazy doc/detail enrichment via completionItem/resolve.
                    // Each item carries a small `data` blob the client
                    // echoes back so the server can rehydrate identity
                    // without recomputing the full completion list.
                    {"resolveProvider", true},
                    // `@` triggers annotation completion; `:` covers
                    // both type annotations and the `::` static-member
                    // operator.
                    {"triggerCharacters", json::array({".", ":", "\"", "/", "@"})}
                }
            },
            {"hoverProvider", true},
            {"definitionProvider", true},
            {"codeActionProvider", true}, // Quick fixes and refactorings
            {
                "codeLensProvider", {
                    {"resolveProvider", false}
                }
            },
            {"documentFormattingProvider", true},
            {"documentRangeFormattingProvider", false}, // TODO: Implement
            {"referencesProvider", true},
            // MYT-294 — `prepareProvider` lets clients call
            // `textDocument/prepareRename` to validate the cursor before
            // popping the rename UI; we return null when the symbol is
            // not renameable so the editor can show a localized error.
            {
                "renameProvider", {
                    {"prepareProvider", true}
                }
            },
            {
                "signatureHelpProvider", {
                    {"triggerCharacters", json::array({"(", ","})}
                }
            },
            {
                "semanticTokensProvider", {
                    {
                        "legend", {
                            {"tokenTypes", SemanticTokensHandler::tokenTypes()},
                            {"tokenModifiers", SemanticTokensHandler::tokenModifiers()}
                        }
                    },
                    {"full", true}
                }
            },
            // MYT-295 — inlay hints (parameter-name + lambda-param type).
            // v1 does not implement `inlayHint/resolve`, so we advertise
            // the boolean form rather than `{resolveProvider: true}`.
            {"inlayHintProvider", true},
            // MYT-296 — document symbols / outline. Hierarchical
            // DocumentSymbol[] is preferred over the flat
            // SymbolInformation[] response, which is what modern clients
            // (VS Code Outline view, breadcrumbs) consume.
            {"documentSymbolProvider", true},
            // MYT-297 — workspace-wide symbol search served by the
            // existing MYT-47 workspace symbol index. Boolean form (no
            // workspaceSymbol/resolve) — responses already carry full
            // locations.
            {"workspaceSymbolProvider", true}
        };

        json result = {
            {"capabilities", capabilities},
            {
                "serverInfo", {
                    {"name", "mType Language Server"},
                    {"version", mType::version::getVersionString()}
                }
            }
        };

        sendResponse(id, result);
    }

    void MTypeLanguageServer::handleShutdown(const json& id)
    {
        sendResponse(id, nullptr);
    }

    void MTypeLanguageServer::handleDidOpenTextDocument(const json& params)
    {
        auto textDocument = params["textDocument"];
        std::string uri = textDocument["uri"];
        std::string text = textDocument["text"];
        int version = textDocument["version"];

        documentManager_->openDocument(uri, text, version);
        diagnosticsHandler_->publishDiagnostics(uri);

        // MYT-47 — keep the workspace symbol index in sync with the
        // newly-parsed document so the missing-import quick fix can
        // surface symbols defined in files that have just been opened.
        // Use the buffer overload so unsaved changes (LSP can reopen a
        // dirty document) are visible to the index immediately.
        if (workspaceIndex_) workspaceIndex_->reindexFile(uri, text);
    }

    void MTypeLanguageServer::handleDidChangeTextDocument(const json& params)
    {
        auto textDocument = params["textDocument"];
        std::string uri = textDocument["uri"];
        int version = textDocument["version"];

        auto contentChanges = params["contentChanges"];
        if (contentChanges.is_array() && !contentChanges.empty())
        {
            std::string text = contentChanges[0]["text"];
            documentManager_->updateDocument(uri, text, version);
            diagnosticsHandler_->publishDiagnostics(uri);
            // MYT-47 — refresh the workspace index entries for this file.
            // Debounced so a typing burst only fires one reindex after the
            // user pauses (full lex+parse on every keystroke is wasteful).
            // The buffer is captured by value so the index sees the user's
            // unsaved changes, not whatever happens to be on disk.
            scheduleDebouncedReindex(uri, text);
        }
    }

    void MTypeLanguageServer::scheduleDebouncedReindex(const std::string& uri,
                                                       const std::string& content)
    {
        if (!workspaceIndex_) return;

        // Capture-by-value: shared_ptrs, a uint64, and the buffer. No
        // `this` capture so an in-flight task survives LSP destruction
        // without dangling.
        auto debouncer = reindexDebouncer_;
        auto idx = workspaceIndex_;
        std::uint64_t version;
        {
            std::lock_guard<std::mutex> lock(debouncer->mutex);
            version = ++debouncer->versions[uri];
        }

        std::thread([uri, content, version, debouncer, idx]()
        {
            std::this_thread::sleep_for(kReindexDebounceWindow);
            {
                std::lock_guard<std::mutex> lock(debouncer->mutex);
                auto it = debouncer->versions.find(uri);
                if (it == debouncer->versions.end() || it->second != version)
                {
                    return;  // a newer change came in; this task is stale.
                }
            }
            // Buffer overload — workspace index sees the live editor
            // content, not whatever's on disk.
            idx->reindexFile(uri, content);
        }).detach();
    }

    void MTypeLanguageServer::handleDidCloseTextDocument(const json& params)
    {
        std::string uri = params["textDocument"]["uri"];
        documentManager_->closeDocument(uri);
        // MYT-47 — drop entries for closed documents so the index doesn't
        // accumulate stale data when the editor closes a file.
        if (workspaceIndex_) workspaceIndex_->invalidateFile(uri);
    }

    void MTypeLanguageServer::handleCompletion(const json& id, const json& params)
    {
        std::string uri = params["textDocument"]["uri"];
        Position position = params["position"];

        auto items = completionHandler_->handleCompletion(uri, position);

        json jsonItems = json::array();
        for (const auto& item : items)
        {
            jsonItems.push_back(item.toJson());
        }

        sendResponse(id, jsonItems);
    }

    void MTypeLanguageServer::handleCompletionItemResolve(const json& id, const json& params)
    {
        // VS Code echoes the original CompletionItem (with the opaque
        // `data` blob the server stamped) so we can rehydrate the
        // symbol identity and fill in documentation lazily. On a
        // malformed request we surface the issue on stderr and reply
        // with a minimal valid CompletionItem rather than echoing the
        // raw client params (which wouldn't be a CompletionItem per
        // the LSP spec and could trip strict clients).
        try
        {
            CompletionItem item = params.get<CompletionItem>();
            CompletionItem resolved = completionHandler_->resolveCompletion(item);
            sendResponse(id, resolved.toJson());
        }
        catch (const std::exception& e)
        {
            std::cerr << "[mtype-lsp] completionItem/resolve failed to parse "
                      << "the client's echoed item: " << e.what() << "\n";
            CompletionItem fallback;
            if (params.is_object() && params.contains("label")
                && params.at("label").is_string())
            {
                fallback.label = params.at("label").get<std::string>();
            }
            if (params.is_object() && params.contains("kind")
                && params.at("kind").is_number_integer())
            {
                fallback.kind = params.at("kind").get<int>();
            }
            else
            {
                fallback.kind = static_cast<int>(CompletionItemKind::Text);
            }
            sendResponse(id, fallback.toJson());
        }
    }

    void MTypeLanguageServer::handleHover(const json& id, const json& params)
    {
        std::string uri = params["textDocument"]["uri"];
        Position position = params["position"];

        auto hover = hoverHandler_->handleHover(uri, position);

        if (hover)
        {
            sendResponse(id, hover->toJson());
        }
        else
        {
            sendResponse(id, nullptr);
        }
    }

    void MTypeLanguageServer::handleDefinition(const json& id, const json& params)
    {
        std::string uri = params["textDocument"]["uri"];
        Position position = params["position"];

        auto location = definitionHandler_->handleDefinition(uri, position);

        if (location)
        {
            sendResponse(id, location->toJson());
        }
        else
        {
            sendResponse(id, nullptr);
        }
    }

    void MTypeLanguageServer::handleCodeAction(const json& id, const json& params)
    {
        std::string uri = params["textDocument"]["uri"];
        Range range = params["range"];

        // Parse the context.diagnostics list. VS Code sends every diagnostic
        // attached to the cursor range here so the server can match each
        // action to a specific diagnostic via the `data` blob and the
        // `code` field. Previously this list was silently dropped.
        std::vector<Diagnostic> diagnostics;
        if (params.contains("context") && params["context"].contains("diagnostics"))
        {
            const auto& diagsJson = params["context"]["diagnostics"];
            if (diagsJson.is_array())
            {
                for (const auto& diagJson : diagsJson)
                {
                    diagnostics.push_back(diagJson.get<Diagnostic>());
                }
            }
        }

        auto actions = codeActionHandler_->handleCodeAction(uri, range, diagnostics);

        json result = json::array();
        for (const auto& action : actions)
        {
            result.push_back(action.toJson());
        }

        sendResponse(id, result);
    }

    void MTypeLanguageServer::handleCodeLens(const json& id, const json& params)
    {
        std::string uri = params["textDocument"]["uri"];

        auto lenses = codeLensHandler_->handleCodeLens(uri);

        json result = json::array();
        for (const auto& lens : lenses)
        {
            result.push_back(lens.toJson());
        }

        sendResponse(id, result);
    }

    void MTypeLanguageServer::handleFormatting(const json& id, const json& params)
    {
        std::string uri = params["textDocument"]["uri"];

        // Get formatting options
        FormattingOptions options;
        if (params.contains("options"))
        {
            auto opts = params["options"];
            if (opts.contains("tabSize"))
            {
                options.tabSize = opts["tabSize"];
            }
            if (opts.contains("insertSpaces"))
            {
                options.insertSpaces = opts["insertSpaces"];
            }
            if (opts.contains("trimTrailingWhitespace"))
            {
                options.trimTrailingWhitespace = opts["trimTrailingWhitespace"];
            }
            if (opts.contains("insertFinalNewline"))
            {
                options.insertFinalNewline = opts["insertFinalNewline"];
            }
        }

        // Get document content
        auto doc = documentManager_->getDocument(uri);
        if (!doc)
        {
            sendResponse(id, json::array());
            return;
        }

        // Format the document
        auto edits = formattingHandler_->formatDocument(doc->content, options);

        // Convert to JSON
        json result = json::array();
        for (const auto& edit : edits)
        {
            result.push_back(edit.toJson());
        }

        sendResponse(id, result);
    }

    void MTypeLanguageServer::handleReferences(const json& id, const json& params)
    {
        std::string uri = params["textDocument"]["uri"];
        Position position = params["position"];

        bool includeDeclaration = false;
        if (params.contains("context") && params["context"].contains("includeDeclaration"))
        {
            includeDeclaration = params["context"]["includeDeclaration"].get<bool>();
        }

        auto locations = referencesHandler_->handleReferences(uri, position, includeDeclaration);

        json result = json::array();
        for (const auto& loc : locations)
        {
            result.push_back(loc.toJson());
        }

        sendResponse(id, result);
    }

    void MTypeLanguageServer::handlePrepareRename(const json& id, const json& params)
    {
        // MYT-294 — `textDocument/prepareRename` validates whether the
        // cursor is on a renameable symbol. When it isn't, the LSP spec
        // accepts a null result (the client suppresses its rename UI);
        // we additionally send a -32602 with a message so editors that
        // surface server-side errors can show "why not". Returning a
        // Range tells the client the exact identifier span to highlight.
        //
        // Malformed `params` (missing `textDocument.uri` or `position`)
        // would otherwise throw an nlohmann exception that the run()
        // loop catches without replying, leaving the editor hanging.
        // Reply -32602 so the client can fail fast.
        try {
            std::string uri = params["textDocument"]["uri"];
            Position position = params["position"];

            auto result = renameHandler_->prepareRename(uri, position);
            if (!result.ok)
            {
                sendError(id, -32602, result.error);
                return;
            }
            sendResponse(id, json{
                {"start", result.range.start},
                {"end", result.range.end}
            });
        } catch (const std::exception&) {
            sendError(id, -32602, "invalid prepareRename request");
        }
    }

    void MTypeLanguageServer::handleRename(const json& id, const json& params)
    {
        // MYT-294 — `textDocument/rename` returns a WorkspaceEdit whose
        // `changes` map is grouped by URI. Each edit's range comes from
        // the lexer's token stream so we never touch comments, string
        // literals, import paths, or any other non-identifier source
        // text.
        //
        // Same defensive shape as handlePrepareRename — reply -32602 on
        // malformed `params` rather than leaving the request hanging.
        try {
            std::string uri = params["textDocument"]["uri"];
            Position position = params["position"];
            std::string newName = params.value("newName", "");

            auto result = renameHandler_->rename(uri, position, newName);
            if (!result.ok)
            {
                sendError(id, -32602, result.error);
                return;
            }
            sendResponse(id, result.edit.toJson());
        } catch (const std::exception&) {
            sendError(id, -32602, "invalid rename request");
        }
    }

    void MTypeLanguageServer::handleSignatureHelp(const json& id, const json& params)
    {
        std::string uri = params["textDocument"]["uri"];
        Position position = params["position"];

        auto help = signatureHelpHandler_->handleSignatureHelp(uri, position);

        if (help)
        {
            sendResponse(id, help->toJson());
        }
        else
        {
            sendResponse(id, nullptr);
        }
    }

    void MTypeLanguageServer::handleSemanticTokensFull(const json& id, const json& params)
    {
        std::string uri = params["textDocument"]["uri"];

        auto tokens = semanticTokensHandler_->handleSemanticTokensFull(uri);
        sendResponse(id, tokens.toJson());
    }

    void MTypeLanguageServer::handleInlayHint(const json& id, const json& params)
    {
        // MYT-295 — `textDocument/inlayHint` returns InlayHint[] for the
        // given Range. The handler returns an empty vector on any
        // partial/unparsed state, so a malformed `range` param is the
        // only failure mode here. On any exception we still reply with
        // an empty array rather than -32603, since the LSP spec allows
        // it and an error reply would clear the editor's existing
        // hints mid-edit.
        try {
            std::string uri = params["textDocument"]["uri"];
            Range range = params["range"].get<Range>();

            auto hints = inlayHintHandler_->handleInlayHint(uri, range);

            json result = json::array();
            for (const auto& h : hints) {
                result.push_back(h.toJson());
            }
            sendResponse(id, result);
        } catch (const std::exception&) {
            sendResponse(id, json::array());
        }
    }

    void MTypeLanguageServer::handleDocumentSymbol(const json& id, const json& params)
    {
        // MYT-296 — `textDocument/documentSymbol` returns a
        // DocumentSymbol[] tree describing the open document's outline.
        // The handler already guards against unparsed / partial state
        // and never throws, but a malformed `params` (e.g., missing
        // textDocument.uri) still has to be caught here so a junk
        // request can't bring down the LSP loop. Empty array on any
        // failure — same defensive shape as handleInlayHint.
        try {
            std::string uri = params["textDocument"]["uri"];
            auto symbols = documentSymbolHandler_->handleDocumentSymbol(uri);

            json result = json::array();
            for (const auto& sym : symbols) {
                result.push_back(sym.toJson());
            }
            sendResponse(id, result);
        } catch (const std::exception&) {
            sendResponse(id, json::array());
        }
    }

    void MTypeLanguageServer::handleWorkspaceSymbol(const json& id, const json& params)
    {
        // MYT-297 — `workspace/symbol` returns a flat SymbolInformation[]
        // of top-level declarations matching `query`. Empty query is
        // legal per spec and returns the full (capped) symbol set.
        // Same defensive shape as handleDocumentSymbol: extract `query`
        // tolerantly (missing or non-string → empty) and serialise an
        // empty array on any exception so a malformed request can't
        // bring down the LSP loop.
        try {
            std::string query;
            if (params.contains("query") && params["query"].is_string()) {
                query = params["query"].get<std::string>();
            }

            auto symbols = workspaceSymbolHandler_->handleWorkspaceSymbol(query);

            json result = json::array();
            for (const auto& sym : symbols) {
                result.push_back(sym.toJson());
            }
            sendResponse(id, result);
        } catch (const std::exception&) {
            sendResponse(id, json::array());
        }
    }

    void MTypeLanguageServer::sendResponse(const json& id, const json& result)
    {
        std::string response = JsonRpc::createResponse(id, result);
        JsonRpc::writeMessage(std::cout, response);
    }

    void MTypeLanguageServer::sendError(const json& id, int code, const std::string& message)
    {
        std::string error = JsonRpc::createError(id, code, message);
        JsonRpc::writeMessage(std::cout, error);
    }

    void MTypeLanguageServer::logMessage(const std::string& message)
    {
        json params = {
            {"type", 4}, // 4 = Log
            {"message", message}
        };
        std::string notification = JsonRpc::createNotification("window/logMessage", params);
        JsonRpc::writeMessage(std::cout, notification);
    }

    void MTypeLanguageServer::publishDiagnostics(const std::string& uri, const std::vector<Diagnostic>& diagnostics)
    {
        json jsonDiagnostics = json::array();
        for (const auto& diag : diagnostics)
        {
            jsonDiagnostics.push_back(diag.toJson());
        }

        json params = {
            {"uri", uri},
            {"diagnostics", jsonDiagnostics}
        };

        std::string notification = JsonRpc::createNotification("textDocument/publishDiagnostics", params);
        JsonRpc::writeMessage(std::cout, notification);
    }
} // namespace mtype::lsp
