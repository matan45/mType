#include "MTypeLanguageServer.hpp"
#include <iostream>

namespace mtype::lsp
{
    MTypeLanguageServer::MTypeLanguageServer()
    {
        documentManager_ = std::make_unique<DocumentManager>();
        completionHandler_ = std::make_unique<CompletionHandler>(documentManager_.get());
        diagnosticsHandler_ = std::make_unique<DiagnosticsHandler>(documentManager_.get());
        hoverHandler_ = std::make_unique<HoverHandler>(documentManager_.get());
        definitionHandler_ = std::make_unique<DefinitionHandler>(documentManager_.get());
        codeActionHandler_ = std::make_unique<CodeActionHandler>(documentManager_.get());
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
        }
    }

    void MTypeLanguageServer::handleDidCloseTextDocument(const json& params)
    {
        std::string uri = params["textDocument"]["uri"];
        documentManager_->closeDocument(uri);
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

        // Get diagnostics if provided
        std::vector<Diagnostic> diagnostics;
        if (params.contains("context") && params["context"].contains("diagnostics"))
        {
            // Parse diagnostics from params if needed
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
