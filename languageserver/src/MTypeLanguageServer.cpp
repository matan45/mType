#include "MTypeLanguageServer.hpp"
#include "utils/UriUtils.hpp"
#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <thread>

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
            catch (const json::parse_error&)
            {
                // Silently ignore parse errors
            }
            catch (const std::exception&)
            {
                // Silently ignore errors
            }
        }
    }

    void MTypeLanguageServer::handleMessage(const json& message)
    {
        if (!message.contains("method"))
        {
            return; // Response message, ignore for now
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
                    {"resolveProvider", false},
                    {"triggerCharacters", json::array({".", ":", "\"", "/"})}
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
            {"referencesProvider", false} // TODO: Implement
        };

        json result = {
            {"capabilities", capabilities},
            {
                "serverInfo", {
                    {"name", "mType Language Server"},
                    {"version", "0.2.0"}
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
