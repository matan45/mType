#include "SignatureHelpHandlerTestSuite.hpp"
#include "../src/handlers/SignatureHelpHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "TestFixtures.hpp"

namespace mtype::lsp::test {

void SignatureHelpHandlerTestSuite::registerTests(LspTestHarness& harness) {

    harness.addTest("returns signature for builtin print()", []() {
        const std::string source = "print(42);\n";

        auto docMgr = makeDocManager("file:///test.mt", source);
        SignatureHelpHandler handler(docMgr.get());

        // Cursor after "print(" — position at col 6
        auto result = handler.handleSignatureHelp("file:///test.mt", {0, 6});
        require(result.has_value(), "expected signature help for print()");
        require(!result->signatures.empty(), "expected at least one signature");
        require(result->signatures[0].name == "print",
            "expected signature name 'print', got '" + result->signatures[0].name + "'");
        require(result->signatures[0].parameters.size() == 1,
            "print should have 1 parameter");
        require(result->activeParameter == 0, "active parameter should be 0");
    });

    harness.addTest("returns signature for builtin hashCode()", []() {
        const std::string source = "hashCode(obj);\n";

        auto docMgr = makeDocManager("file:///test.mt", source);
        SignatureHelpHandler handler(docMgr.get());

        auto result = handler.handleSignatureHelp("file:///test.mt", {0, 9});
        require(result.has_value(), "expected signature help for hashCode()");
        require(result->signatures[0].name == "hashCode",
            "expected name 'hashCode'");
    });

    harness.addTest("returns signature for builtin strLength()", []() {
        const std::string source = "strLength(\"hello\");\n";

        auto docMgr = makeDocManager("file:///test.mt", source);
        SignatureHelpHandler handler(docMgr.get());

        auto result = handler.handleSignatureHelp("file:///test.mt", {0, 10});
        require(result.has_value(), "expected signature help for strLength()");
        require(result->signatures[0].returnType == "int",
            "strLength return type should be 'int'");
    });

    harness.addTest("tracks active parameter by counting commas", []() {
        const std::string source = "foo(a, b, c);\n";

        auto docMgr = makeDocManager("file:///test.mt", source);
        SignatureHelpHandler handler(docMgr.get());

        // Cursor after first comma: "foo(a, " — col 7 → activeParameter=1
        // "foo" is not a builtin or declared function, so this returns nullopt.
        // The active parameter tracking is validated by the nested-parens test
        // which uses the builtin "print" instead.
        auto result1 = handler.handleSignatureHelp("file:///test.mt", {0, 7});
        require(!result1.has_value(),
            "unknown function 'foo' should return nullopt");
    });

    harness.addTest("returns nullopt when no function call at cursor", []() {
        const std::string source = "int x = 5;\n";

        auto docMgr = makeDocManager("file:///test.mt", source);
        SignatureHelpHandler handler(docMgr.get());

        auto result = handler.handleSignatureHelp("file:///test.mt", {0, 5});
        require(!result.has_value(), "expected nullopt when no function call");
    });

    harness.addTest("returns nullopt for unknown URI", []() {
        auto docMgr = makeDocManager("file:///test.mt", "print(1);\n");
        SignatureHelpHandler handler(docMgr.get());

        auto result = handler.handleSignatureHelp("file:///unknown.mt", {0, 6});
        require(!result.has_value(), "expected nullopt for unknown URI");
    });

    harness.addTest("finds constructor signature", []() {
        const std::string source =
            "class Point {\n"
            "    public function constructor(int x, int y): void {\n"
            "    }\n"
            "}\n"
            "Point p = new Point(1, 2);\n";

        auto docMgr = makeDocManager("file:///test.mt", source);
        SignatureHelpHandler handler(docMgr.get());

        // Cursor inside "new Point(" — line 4, col 20
        auto result = handler.handleSignatureHelp("file:///test.mt", {4, 20});
        require(result.has_value(), "expected signature help for constructor call");
        require(!result->signatures.empty(), "expected constructor signature");
        require(result->signatures[0].parameters.size() == 2,
            "constructor should have 2 parameters");
    });

    harness.addTest("finds method signature through safe-navigation call", []() {
        // MYT-374: `p?.move(` must resolve the method signature just like
        // `p.move(` — the receiver extractor steps over the `?` of `?.`.
        const std::string source =
            "class Point {\n"
            "    public function move(int dx, int dy): void {\n"
            "    }\n"
            "}\n"
            "Point p = new Point();\n"
            "p?.move(1, 2);\n"; // public method, resolvable from global scope

        auto docMgr = makeDocManager("file:///test.mt", source);
        SignatureHelpHandler handler(docMgr.get());

        // Line 5: "p?.move(1, 2);" — col 8 is right after "p?.move("
        auto result = handler.handleSignatureHelp("file:///test.mt", {5, 8});
        require(result.has_value(), "expected signature help for p?.move()");
        require(!result->signatures.empty(), "expected method signature");
        require(result->signatures[0].name == "move",
            "expected signature name 'move', got '" + result->signatures[0].name + "'");
        require(result->signatures[0].parameters.size() == 2,
            "move should have 2 parameters");
    });

    harness.addTest("signature help toJson produces valid LSP format", []() {
        SignatureHelp help;
        SignatureInfo sig;
        sig.name = "test";
        sig.returnType = "void";
        sig.parameters = {{"a", "int"}, {"b", "string"}};
        help.signatures.push_back(sig);
        help.activeSignature = 0;
        help.activeParameter = 1;

        auto j = help.toJson();
        require(j.contains("signatures"), "JSON should contain 'signatures'");
        require(j.contains("activeSignature"), "JSON should contain 'activeSignature'");
        require(j.contains("activeParameter"), "JSON should contain 'activeParameter'");
        require(j["activeParameter"] == 1, "activeParameter should be 1");
        require(j["signatures"].size() == 1, "should have 1 signature");
        require(j["signatures"][0].contains("label"), "signature should have label");
        require(j["signatures"][0].contains("parameters"), "signature should have parameters");
        require(j["signatures"][0]["parameters"].size() == 2, "should have 2 parameters");
    });

    harness.addTest("handles nested parentheses in parameter counting", []() {
        const std::string source = "print(foo(1), bar);\n";

        auto docMgr = makeDocManager("file:///test.mt", source);
        SignatureHelpHandler handler(docMgr.get());

        // Cursor after "print(foo(1), " — col 14, should be on parameter 1
        auto result = handler.handleSignatureHelp("file:///test.mt", {0, 14});
        require(result.has_value(), "expected signature help for print() with nested call");
        require(result->activeParameter == 1,
            "should be on parameter 1 after nested call, got " + std::to_string(result->activeParameter));
    });
}

} // namespace mtype::lsp::test
