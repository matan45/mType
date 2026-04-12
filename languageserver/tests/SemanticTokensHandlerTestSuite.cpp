#include "SemanticTokensHandlerTestSuite.hpp"
#include "../src/handlers/SemanticTokensHandler.hpp"
#include "../src/DocumentManager.hpp"
#include "TestFixtures.hpp"

#include <algorithm>

namespace mtype::lsp::test {

// Helper: decode delta-encoded tokens back into absolute positions
struct DecodedToken {
    int line;
    int startChar;
    int length;
    int tokenType;
    int tokenModifiers;
};

static std::vector<DecodedToken> decodeTokens(const SemanticTokens& tokens) {
    std::vector<DecodedToken> result;
    int prevLine = 0;
    int prevStart = 0;

    for (size_t i = 0; i + 4 < tokens.data.size(); i += 5) {
        int deltaLine = static_cast<int>(tokens.data[i]);
        int deltaStart = static_cast<int>(tokens.data[i + 1]);
        int length = static_cast<int>(tokens.data[i + 2]);
        int tokenType = static_cast<int>(tokens.data[i + 3]);
        int tokenMods = static_cast<int>(tokens.data[i + 4]);

        int line = prevLine + deltaLine;
        int startChar = (deltaLine == 0) ? (prevStart + deltaStart) : deltaStart;

        result.push_back({line, startChar, length, tokenType, tokenMods});
        prevLine = line;
        prevStart = startChar;
    }

    return result;
}

// Helper: find a token that matches criteria
static bool hasToken(const std::vector<DecodedToken>& tokens,
                     int line, int tokenType, int minLength = 1) {
    return std::any_of(tokens.begin(), tokens.end(),
        [&](const DecodedToken& t) {
            return t.line == line && t.tokenType == tokenType && t.length >= minLength;
        });
}

void SemanticTokensHandlerTestSuite::registerTests(LspTestHarness& harness) {

    harness.addTest("legend has correct number of token types", []() {
        const auto& types = SemanticTokensHandler::tokenTypes();
        require(types.size() == 20,
            "expected 20 token types, got " + std::to_string(types.size()));
        require(types[0] == "namespace", "first type should be 'namespace'");
        require(types[19] == "decorator", "last type should be 'decorator'");
    });

    harness.addTest("legend has correct number of token modifiers", []() {
        const auto& mods = SemanticTokensHandler::tokenModifiers();
        require(mods.size() == 10,
            "expected 10 token modifiers, got " + std::to_string(mods.size()));
        require(mods[0] == "declaration", "first modifier should be 'declaration'");
        require(mods[9] == "defaultLibrary", "last modifier should be 'defaultLibrary'");
    });

    harness.addTest("returns empty tokens for empty document", []() {
        auto docMgr = makeDocManager("file:///test.mt", "");
        SemanticTokensHandler handler(docMgr.get());

        auto tokens = handler.handleSemanticTokensFull("file:///test.mt");
        require(tokens.data.empty(), "expected no tokens for empty document");
    });

    harness.addTest("returns empty tokens for unknown URI", []() {
        auto docMgr = makeDocManager("file:///test.mt", "class Foo {}\n");
        SemanticTokensHandler handler(docMgr.get());

        auto tokens = handler.handleSemanticTokensFull("file:///unknown.mt");
        require(tokens.data.empty(), "expected no tokens for unknown URI");
    });

    harness.addTest("tokenizes annotations", []() {
        auto docMgr = makeDocManager("file:///test.mt", "@Override\n@Script\n");
        SemanticTokensHandler handler(docMgr.get());

        auto tokens = handler.handleSemanticTokensFull("file:///test.mt");
        auto decoded = decodeTokens(tokens);

        int decoratorType = 19; // "decorator" index
        require(hasToken(decoded, 0, decoratorType),
            "expected decorator token on line 0 for @Override");
        require(hasToken(decoded, 1, decoratorType),
            "expected decorator token on line 1 for @Script");
    });

    harness.addTest("tokenizes class declaration", []() {
        auto docMgr = makeDocManager("file:///test.mt", "class MyClass {\n}\n");
        SemanticTokensHandler handler(docMgr.get());

        auto tokens = handler.handleSemanticTokensFull("file:///test.mt");
        auto decoded = decodeTokens(tokens);

        int keywordType = 12; // "keyword"
        int classType = 1;    // "class"

        require(hasToken(decoded, 0, keywordType, 5),
            "expected keyword token for 'class'");
        require(hasToken(decoded, 0, classType, 7),
            "expected class token for 'MyClass'");
    });

    harness.addTest("tokenizes interface declaration", []() {
        auto docMgr = makeDocManager("file:///test.mt", "interface Drawable {\n}\n");
        SemanticTokensHandler handler(docMgr.get());

        auto tokens = handler.handleSemanticTokensFull("file:///test.mt");
        auto decoded = decodeTokens(tokens);

        int interfaceType = 3; // "interface"
        require(hasToken(decoded, 0, interfaceType, 8),
            "expected interface token for 'Drawable'");
    });

    harness.addTest("tokenizes method declaration with modifiers", []() {
        auto docMgr = makeDocManager("file:///test.mt",
            "class Foo {\n"
            "    static async function doWork() {}\n"
            "}\n");
        SemanticTokensHandler handler(docMgr.get());

        auto tokens = handler.handleSemanticTokensFull("file:///test.mt");
        auto decoded = decodeTokens(tokens);

        int modifierType = 13; // "modifier"
        int keywordType = 12;  // "keyword"
        int methodType = 11;   // "method"

        require(hasToken(decoded, 1, modifierType),
            "expected modifier token on line 1 for 'static'");
        require(hasToken(decoded, 1, keywordType, 8),
            "expected keyword token for 'function'");
        require(hasToken(decoded, 1, methodType, 6),
            "expected method token for 'doWork'");
    });

    harness.addTest("tokenizes keywords", []() {
        auto docMgr = makeDocManager("file:///test.mt", "if (true) { return; }\n");
        SemanticTokensHandler handler(docMgr.get());

        auto tokens = handler.handleSemanticTokensFull("file:///test.mt");
        auto decoded = decodeTokens(tokens);

        int keywordType = 12;
        require(hasToken(decoded, 0, keywordType, 2),
            "expected keyword token for 'if'");
        require(hasToken(decoded, 0, keywordType, 6),
            "expected keyword token for 'return'");
    });

    harness.addTest("tokenizes function calls", []() {
        auto docMgr = makeDocManager("file:///test.mt", "print(42);\nfoo();\n");
        SemanticTokensHandler handler(docMgr.get());

        auto tokens = handler.handleSemanticTokensFull("file:///test.mt");
        auto decoded = decodeTokens(tokens);

        int functionType = 10; // "function"
        require(hasToken(decoded, 0, functionType, 5),
            "expected function token for 'print'");
        require(hasToken(decoded, 1, functionType, 3),
            "expected function token for 'foo'");
    });

    harness.addTest("does not tokenize control keywords as function calls", []() {
        auto docMgr = makeDocManager("file:///test.mt", "if (x) {}\nwhile (true) {}\n");
        SemanticTokensHandler handler(docMgr.get());

        auto tokens = handler.handleSemanticTokensFull("file:///test.mt");
        auto decoded = decodeTokens(tokens);

        int functionType = 10; // "function"
        // "if" and "while" should NOT be tokenized as function calls
        bool ifAsFunction = std::any_of(decoded.begin(), decoded.end(),
            [&](const DecodedToken& t) {
                return t.tokenType == functionType && t.length == 2 && t.line == 0;
            });
        require(!ifAsFunction, "'if' should not be tokenized as function call");
    });

    harness.addTest("toJson produces valid LSP semantic tokens format", []() {
        SemanticTokens tokens;
        tokens.data = {0, 0, 5, 12, 0, 0, 6, 3, 1, 1}; // two tokens

        auto j = tokens.toJson();
        require(j.contains("data"), "JSON should contain 'data'");
        require(j["data"].size() == 10, "data array should have 10 elements");
    });

    harness.addTest("delta encoding is correct", []() {
        auto docMgr = makeDocManager("file:///test.mt",
            "class A {}\n"
            "class B {}\n");
        SemanticTokensHandler handler(docMgr.get());

        auto tokens = handler.handleSemanticTokensFull("file:///test.mt");
        require(tokens.data.size() % 5 == 0,
            "token data length should be multiple of 5");

        // Verify all decoded tokens have non-negative positions
        auto decoded = decodeTokens(tokens);
        for (const auto& t : decoded) {
            require(t.line >= 0, "line should be non-negative");
            require(t.startChar >= 0, "startChar should be non-negative");
            require(t.length > 0, "length should be positive");
        }
    });
}

} // namespace mtype::lsp::test
