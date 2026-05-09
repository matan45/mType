// MYT-291: headless guard for the code-editor LSP startup JSON path.
// The GUI startup builds an initialize JSON-RPC envelope, then later parses
// LSP responses through JsonNode/JsonCursor. Repeating both paths makes the
// JsonCursor::skipWs OR-chain loop hot enough for JIT/OSR decisions without
// requiring an SDL window in the test runner.

import * from "../../../../../examples/code-editor/JsonNode.mt";

function buildInitializeBody(int id): string {
    JsonNode capabilities = JsonNode::objectNode();
    capabilities.set("textDocument", JsonNode::objectNode());
    capabilities.set("workspace", JsonNode::objectNode());

    JsonNode clientInfo = JsonNode::objectNode();
    clientInfo.setString("name", "mType-code-editor");
    clientInfo.setString("version", "0.1.0");

    JsonNode initParams = JsonNode::objectNode();
    initParams.setInt("processId", 0);
    initParams.setString("rootUri", "file:///C:/matan/mType");
    initParams.setString("rootPath", "C:\\matan\\mType");
    initParams.set("capabilities", capabilities);
    initParams.set("clientInfo", clientInfo);

    JsonNode env = JsonNode::objectNode();
    env.setString("jsonrpc", "2.0");
    env.setInt("id", id);
    env.setString("method", "initialize");
    env.set("params", initParams);
    return env.toJsonString();
}

function exerciseOutbound(): bool {
    string last = "";
    for (int i = 0; i < 200; i = i + 1) {
        last = buildInitializeBody(i + 1);
    }
    return indexOf(last, "initialize") >= 0
        && indexOf(last, "mType-code-editor") >= 0;
}

function exerciseInbound(): bool {
    string response = "   \t\r\n{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"capabilities\":{},\"serverInfo\":{\"name\":\"mtype-lsp\"}}}";
    for (int i = 0; i < 200; i = i + 1) {
        JsonNode msg = JsonNode::parse(response);
        if (!msg.isObject()) return false;
        if (msg.getInt("id") != 1) return false;
        JsonNode result = msg.getObject("result");
        if (!result.isObject()) return false;
    }
    return true;
}

function main(): void {
    if (exerciseOutbound() && exerciseInbound()) {
        print("ok");
    } else {
        print("fail");
    }
}

main();
