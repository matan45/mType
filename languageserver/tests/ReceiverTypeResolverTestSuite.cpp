#include "ReceiverTypeResolverTestSuite.hpp"
#include "../src/DocumentManager.hpp"
#include "../src/analysis/ReceiverTypeResolver.hpp"
#include "../src/analysis/AstPositionIndex.hpp"
#include "../../mType/ast/nodes/expressions/VariableNode.hpp"
#include "../../mType/ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../mType/ast/nodes/classes/MethodCallNode.hpp"
#include "../../mType/ast/nodes/classes/NewNode.hpp"
#include "TestFixtures.hpp"

namespace mtype::lsp::test {

namespace {

// Shared source: a class with a fluent method, a free function with a
// parameter, locals of class/array type, and a ternary receiver. Line
// numbers (0-based) are load-bearing for the cursor-positioned resolvers.
const char* kSource =
    "class Foo {\n"                                  // 0
    "    public function bar(): Foo {\n"             // 1
    "        return this;\n"                         // 2
    "    }\n"                                        // 3
    "}\n"                                            // 4
    "function takesFoo(Foo p): void {\n"             // 5
    "    p.bar();\n"                                 // 6
    "}\n"                                            // 7
    "function main(): void {\n"                      // 8
    "    Foo f = new Foo();\n"                       // 9
    "    Foo g = new Foo();\n"                       // 10
    "    int[] xs = new int[3];\n"                   // 11
    "    bool flag = true;\n"                        // 12
    "    f.bar();\n"                                 // 13
    "    (flag ? f : g).bar();\n"                    // 14
    "}\n"                                            // 15
    "main();\n";                                     // 16

// Finds the first AST node dynamic_castable to T, searching the whole
// document. Good enough here because each fixture uses each node shape once
// (or the first occurrence is the one under test).
template <typename T>
const T* findFirst(const std::vector<std::unique_ptr<ast::ASTNode>>& roots) {
    const T* result = nullptr;
    walkAst(roots, [&](const ast::ASTNode* n) {
        if (auto* typed = dynamic_cast<const T*>(n)) {
            result = typed;
            return true;
        }
        return false;
    });
    return result;
}

} // namespace

void ReceiverTypeResolverTestSuite::registerTests(LspTestHarness& harness) {

    // === unifiedTypeFromClassName (pure helper) ===

    harness.addTest("unifiedTypeFromClassName builds class type", []() {
        auto t = unifiedTypeFromClassName("Foo");
        require(t != nullptr, "expected non-null type for 'Foo'");
        require(t->isClass(), "'Foo' should resolve to a class type");
        require(t->toString().find("Foo") != std::string::npos,
            "type string should mention Foo, got '" + t->toString() + "'");
    });

    harness.addTest("unifiedTypeFromClassName builds array type for [] suffix", []() {
        auto t = unifiedTypeFromClassName("Foo[]");
        require(t != nullptr, "expected non-null type for 'Foo[]'");
        require(t->isArray(), "'Foo[]' should resolve to an array type");
    });

    harness.addTest("unifiedTypeFromClassName returns null for empty input", []() {
        auto t = unifiedTypeFromClassName("");
        require(t == nullptr, "expected nullptr for empty class name");
    });

    // === lookupVariableType (string-keyed fallback chain) ===

    harness.addTest("lookupVariableType resolves class-typed local", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr && doc->environment != nullptr, "doc/env missing");

        ReceiverTypeResolver resolver(doc->environment, &doc->ast, 13, 4);
        auto t = resolver.lookupVariableType("f");
        require(t != nullptr, "expected a type for local 'f'");
        require(t->toString().find("Foo") != std::string::npos,
            "expected Foo, got '" + t->toString() + "'");
    });

    harness.addTest("lookupVariableType resolves array-typed local", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr && doc->environment != nullptr, "doc/env missing");

        ReceiverTypeResolver resolver(doc->environment, &doc->ast, 13, 4);
        auto t = resolver.lookupVariableType("xs");
        require(t != nullptr, "expected a type for local 'xs'");
        require(t->isArray(), "expected array type for 'xs', got '"
            + t->toString() + "'");
    });

    harness.addTest("lookupVariableType resolves parameter inside its function", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr && doc->environment != nullptr, "doc/env missing");

        // Cursor inside takesFoo's body (line 6).
        ReceiverTypeResolver resolver(doc->environment, &doc->ast, 6, 4);
        auto t = resolver.lookupVariableType("p");
        require(t != nullptr, "expected a type for parameter 'p'");
        require(t->toString().find("Foo") != std::string::npos,
            "expected Foo, got '" + t->toString() + "'");
    });

    harness.addTest("lookupVariableType returns null for unknown name", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr && doc->environment != nullptr, "doc/env missing");

        ReceiverTypeResolver resolver(doc->environment, &doc->ast, 13, 4);
        auto t = resolver.lookupVariableType("doesNotExist");
        require(t == nullptr, "expected nullptr for unknown variable");
    });

    // === resolve() over receiver AST shapes ===

    harness.addTest("resolve NewNode yields the constructed class", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr && doc->environment != nullptr, "doc/env missing");

        const auto* newNode =
            findFirst<ast::nodes::classes::NewNode>(doc->ast);
        require(newNode != nullptr, "expected a NewNode in the AST");

        ReceiverTypeResolver resolver(doc->environment, &doc->ast, 9, 4);
        require(resolver.resolveName(newNode) == "Foo",
            "new Foo() should resolve to 'Foo'");
    });

    harness.addTest("resolve VariableNode yields its declared type", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr && doc->environment != nullptr, "doc/env missing");

        // First VariableNode named "f" anywhere in the document.
        const ast::nodes::expressions::VariableNode* var = nullptr;
        walkAst(doc->ast, [&](const ast::ASTNode* n) {
            if (auto* v = dynamic_cast<const ast::nodes::expressions::VariableNode*>(n)) {
                if (v->getName() == "f") { var = v; return true; }
            }
            return false;
        });
        require(var != nullptr, "expected a VariableNode named 'f'");

        ReceiverTypeResolver resolver(doc->environment, &doc->ast, 13, 4);
        require(resolver.resolveName(var) == "Foo",
            "variable 'f' should resolve to 'Foo'");
    });

    harness.addTest("resolve MethodCallNode yields the method return type", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr && doc->environment != nullptr, "doc/env missing");

        // f.bar() at line 13, 'bar' anchored at column 6.
        const auto* call = findMethodCallAt(doc->ast, 13, 6, "bar");
        require(call != nullptr, "expected f.bar() MethodCallNode at 13:6");

        ReceiverTypeResolver resolver(doc->environment, &doc->ast, 13, 6);
        require(resolver.resolveName(call) == "Foo",
            "f.bar() should resolve to return type 'Foo'");
    });

    harness.addTest("resolve ternary receiver yields the branch type", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr && doc->environment != nullptr, "doc/env missing");

        const auto* ternary =
            findFirst<ast::nodes::expressions::TernaryExpNode>(doc->ast);
        require(ternary != nullptr, "expected a TernaryExpNode in the AST");

        ReceiverTypeResolver resolver(doc->environment, &doc->ast, 14, 4);
        require(resolver.resolveName(ternary) == "Foo",
            "(flag ? f : g) should resolve to 'Foo'");
    });

    harness.addTest("resolveName returns empty for unresolvable receiver", []() {
        // A document whose only variable has no resolvable declaration from
        // the resolver's viewpoint: reference to an undeclared name still
        // parses best-effort in the LSP path.
        auto docMgr = makeDocManager("file:///t.mt",
            "function main(): void {\n"
            "    mystery.bar();\n"
            "}\n"
            "main();\n");
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr && doc->environment != nullptr, "doc/env missing");

        const ast::nodes::expressions::VariableNode* var = nullptr;
        walkAst(doc->ast, [&](const ast::ASTNode* n) {
            if (auto* v = dynamic_cast<const ast::nodes::expressions::VariableNode*>(n)) {
                if (v->getName() == "mystery") { var = v; return true; }
            }
            return false;
        });
        if (var == nullptr) {
            // Parser may not produce a VariableNode for the unresolved
            // receiver shape — nothing to assert against in that case.
            return;
        }

        ReceiverTypeResolver resolver(doc->environment, &doc->ast, 1, 4);
        require(resolver.resolveName(var).empty(),
            "undeclared receiver should resolve to empty name");
    });
}

} // namespace mtype::lsp::test
