#include "DocumentSymbolHandlerTestSuite.hpp"

#include <string>

#include "../src/DocumentManager.hpp"
#include "../src/handlers/DocumentSymbolHandler.hpp"
#include "../src/utils/LSPTypes.hpp"
#include "TestFixtures.hpp"

namespace mtype::lsp::test {

namespace {

const DocumentSymbol* findByKind(const std::vector<DocumentSymbol>& syms,
                                 SymbolKind kind) {
    for (const auto& s : syms) {
        if (s.kind == kind) return &s;
    }
    return nullptr;
}

const DocumentSymbol* findByName(const std::vector<DocumentSymbol>& syms,
                                 const std::string& name) {
    for (const auto& s : syms) {
        if (s.name == name) return &s;
    }
    return nullptr;
}

int countByKind(const std::vector<DocumentSymbol>& syms, SymbolKind kind) {
    int n = 0;
    for (const auto& s : syms) {
        if (s.kind == kind) n++;
    }
    return n;
}

} // namespace

void DocumentSymbolHandlerTestSuite::registerTests(LspTestHarness& harness) {

    harness.addTest("empty document returns empty symbol list", []() {
        auto docMgr = makeDocManager("file:///t.mt", "");
        DocumentSymbolHandler handler(docMgr.get());
        auto syms = handler.handleDocumentSymbol("file:///t.mt");
        require(syms.empty(), "expected no symbols for empty document");
    });

    harness.addTest("unknown URI returns empty without throwing", []() {
        auto docMgr = makeDocManager("file:///t.mt", "");
        DocumentSymbolHandler handler(docMgr.get());
        auto syms = handler.handleDocumentSymbol("file:///does-not-exist.mt");
        require(syms.empty(), "expected no symbols for unknown URI");
    });

    harness.addTest("top-level class with constructor, field, and method", []() {
        const std::string src =
            "class Foo {\n"
            "    public int x;\n"
            "    public constructor(int v) {}\n"
            "    public function add(int n): int { return 0; }\n"
            "}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        DocumentSymbolHandler handler(docMgr.get());

        auto syms = handler.handleDocumentSymbol("file:///t.mt");
        require(syms.size() == 1,
            "expected 1 top-level symbol, got " + std::to_string(syms.size()));

        const auto& cls = syms[0];
        require(cls.kind == SymbolKind::Class, "expected Class kind at root");
        require(cls.name == "Foo", "expected class name 'Foo', got '" + cls.name + "'");

        // Children: 1 ctor + 1 field + 1 method = 3.
        require(cls.children.size() == 3,
            "expected 3 children, got " + std::to_string(cls.children.size()));

        require(countByKind(cls.children, SymbolKind::Constructor) == 1,
            "expected 1 Constructor child");
        require(countByKind(cls.children, SymbolKind::Field) == 1,
            "expected 1 Field child");
        require(countByKind(cls.children, SymbolKind::Method) == 1,
            "expected 1 Method child");

        const auto* ctor = findByKind(cls.children, SymbolKind::Constructor);
        require(ctor && ctor->name == "Foo",
            "expected constructor to be named after the class");
        require(ctor && ctor->detail && *ctor->detail == "(1)",
            "expected constructor detail '(1)'");

        const auto* method = findByName(cls.children, "add");
        require(method && method->kind == SymbolKind::Method,
            "expected method 'add' with Method kind");
        require(method && method->detail
            && method->detail->find("(1)") != std::string::npos,
            "expected method detail to contain '(1)'");
    });

    harness.addTest("interface with method signatures", []() {
        const std::string src =
            "interface Drawable {\n"
            "    function draw(): void;\n"
            "    function area(): int;\n"
            "}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        DocumentSymbolHandler handler(docMgr.get());

        auto syms = handler.handleDocumentSymbol("file:///t.mt");
        require(syms.size() == 1,
            "expected 1 top-level symbol, got " + std::to_string(syms.size()));
        require(syms[0].kind == SymbolKind::Interface,
            "expected Interface kind at root");
        require(syms[0].name == "Drawable",
            "expected interface name 'Drawable'");
        require(syms[0].children.size() == 2,
            "expected 2 method-signature children");
        require(countByKind(syms[0].children, SymbolKind::Method) == 2,
            "expected both children to have Method kind");
    });

    harness.addTest("top-level function appears at root", []() {
        const std::string src =
            "function greet(string name): void { }\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        DocumentSymbolHandler handler(docMgr.get());

        auto syms = handler.handleDocumentSymbol("file:///t.mt");
        require(syms.size() == 1, "expected 1 top-level symbol");
        require(syms[0].kind == SymbolKind::Function,
            "expected Function kind");
        require(syms[0].name == "greet", "expected function name 'greet'");
        require(syms[0].detail && syms[0].detail->find("(1)") != std::string::npos,
            "expected function detail to contain '(1)'");
        require(syms[0].children.empty(),
            "expected top-level function to have no children");
    });

    harness.addTest("multiple top-level decls preserved in source order", []() {
        const std::string src =
            "function first(): void { }\n"
            "class Middle { public constructor() {} }\n"
            "interface Last { function tail(): void; }\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        DocumentSymbolHandler handler(docMgr.get());

        auto syms = handler.handleDocumentSymbol("file:///t.mt");
        require(syms.size() == 3, "expected 3 top-level symbols");
        require(syms[0].name == "first" && syms[0].kind == SymbolKind::Function,
            "expected first symbol to be the function");
        require(syms[1].name == "Middle" && syms[1].kind == SymbolKind::Class,
            "expected second symbol to be the class");
        require(syms[2].name == "Last" && syms[2].kind == SymbolKind::Interface,
            "expected third symbol to be the interface");
    });

    harness.addTest("static method gets 'static' in detail", []() {
        const std::string src =
            "class Util {\n"
            "    public constructor() {}\n"
            "    public static function helper(int x): int { return x; }\n"
            "}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        DocumentSymbolHandler handler(docMgr.get());

        auto syms = handler.handleDocumentSymbol("file:///t.mt");
        require(!syms.empty() && syms[0].kind == SymbolKind::Class,
            "expected a class at root");
        const auto* m = findByName(syms[0].children, "helper");
        require(m != nullptr, "expected to find method 'helper'");
        require(m->detail && m->detail->find("static") != std::string::npos,
            "expected method detail to contain 'static', got '"
            + (m->detail ? *m->detail : "<none>") + "'");
    });

    harness.addTest("selectionRange points to the class name token", []() {
        const std::string src = "class FooBar { public constructor() {} }\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        DocumentSymbolHandler handler(docMgr.get());

        auto syms = handler.handleDocumentSymbol("file:///t.mt");
        require(syms.size() == 1 && syms[0].name == "FooBar",
            "expected one class named FooBar");
        const auto& sel = syms[0].selectionRange;
        require(sel.start.line == 0,
            "expected selectionRange on line 0");
        // "class " is 6 characters, so 'F' of FooBar starts at column 6.
        require(sel.start.character == 6,
            "expected selectionRange.start.character == 6, got "
            + std::to_string(sel.start.character));
        require(sel.end.character == sel.start.character + 6,
            "expected selectionRange to span 6 chars (length of 'FooBar')");
    });

    harness.addTest("range covers the full multi-line class body", []() {
        const std::string src =
            "class Multi {\n"
            "    public constructor() {}\n"
            "    public function m(): void { }\n"
            "}\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        DocumentSymbolHandler handler(docMgr.get());

        auto syms = handler.handleDocumentSymbol("file:///t.mt");
        require(syms.size() == 1, "expected one class");
        const auto& r = syms[0].range;
        require(r.start.line == 0, "expected range to start on line 0");
        // Body closes on line 3 (zero-based: `}` is on line 3).
        require(r.end.line >= 3,
            "expected range.end.line >= 3 (covers closing brace), got "
            + std::to_string(r.end.line));
    });

    harness.addTest("malformed source does not throw and returns best-effort", []() {
        // Unterminated class body. The parser will throw and
        // DocumentManager keeps the previous AST (empty here), so the
        // handler should return an empty vector — never propagate the
        // exception up to the LSP transport.
        const std::string src = "class Broken {\n    public int x;\n";
        auto docMgr = makeDocManager("file:///t.mt", src);
        DocumentSymbolHandler handler(docMgr.get());

        // require() turns any escaped exception into a test failure with
        // a useful message, so the absence of an explicit try/catch IS
        // the assertion that handleDocumentSymbol does not throw.
        auto syms = handler.handleDocumentSymbol("file:///t.mt");
        // We don't assert empty vs non-empty — best-effort means "either
        // is acceptable as long as we don't crash". In practice with a
        // first-parse failure the AST is empty and `syms` is too.
        require(true, "handler returned without throwing");
        (void)syms;
    });
}

} // namespace mtype::lsp::test
