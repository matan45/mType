#include "JsonRpcTestSuite.hpp"
#include "../src/utils/JsonRpc.hpp"

#include <sstream>

namespace mtype::lsp::test {

void JsonRpcTestSuite::registerTests(LspTestHarness& harness) {

    harness.addTest("createResponse produces valid JSON-RPC 2.0", []() {
        std::string msg = JsonRpc::createResponse(1, json{{"items", json::array()}});
        json j = json::parse(msg);
        require(j["jsonrpc"] == "2.0", "jsonrpc field mismatch");
        require(j["id"] == 1, "id mismatch");
        require(j.contains("result"), "result field missing");
        require(j["result"]["items"].is_array(), "result.items should be array");
    });

    harness.addTest("createError produces error object with code and message", []() {
        std::string msg = JsonRpc::createError(42, -32601, "Method not found");
        json j = json::parse(msg);
        require(j["jsonrpc"] == "2.0", "jsonrpc mismatch");
        require(j["id"] == 42, "id mismatch");
        require(j["error"]["code"] == -32601, "error code mismatch");
        require(j["error"]["message"] == "Method not found", "error message mismatch");
    });

    harness.addTest("createNotification has no id field", []() {
        std::string msg = JsonRpc::createNotification("textDocument/didOpen", json::object());
        json j = json::parse(msg);
        require(j["jsonrpc"] == "2.0", "jsonrpc mismatch");
        require(j["method"] == "textDocument/didOpen", "method mismatch");
        require(!j.contains("id"), "notification should not have id");
    });

    harness.addTest("readMessage parses Content-Length header and body", []() {
        std::string body = R"({"test":"value"})";
        std::stringstream ss;
        ss << "Content-Length: " << body.size() << "\r\n\r\n" << body;

        std::string result = JsonRpc::readMessage(ss);
        require(result == body, "body mismatch: got '" + result + "'");
    });

    harness.addTest("readMessage returns empty on zero content length", []() {
        std::stringstream ss;
        ss << "\r\n";
        std::string result = JsonRpc::readMessage(ss);
        require(result.empty(), "expected empty string for no Content-Length");
    });

    harness.addTest("writeMessage outputs correct Content-Length header", []() {
        std::string body = R"({"id":1})";
        std::stringstream ss;
        JsonRpc::writeMessage(ss, body);

        std::string output = ss.str();
        std::string expectedHeader = "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
        require(output.find(expectedHeader) == 0, "header mismatch");
        require(output.substr(expectedHeader.size()) == body, "body mismatch");
    });
}

} // namespace mtype::lsp::test
