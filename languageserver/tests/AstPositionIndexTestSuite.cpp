#include "AstPositionIndexTestSuite.hpp"
#include "../src/DocumentManager.hpp"
#include "../src/analysis/AstPositionIndex.hpp"
#include "../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../mType/ast/nodes/classes/MethodCallNode.hpp"
#include "../../mType/ast/nodes/functions/FunctionCallNode.hpp"
#include "TestFixtures.hpp"

namespace mtype::lsp::test {

namespace {

// Line numbers (0-based) are load-bearing: the position-index functions
// match the cursor against the node's parse anchor (the name token).
const char* kSource =
    "class Foo {\n"                                  // 0
    "    public function bar(): Foo {\n"             // 1
    "        return this;\n"                         // 2
    "    }\n"                                        // 3
    "}\n"                                            // 4
    "function helper(): int {\n"                     // 5
    "    return 3;\n"                                // 6
    "}\n"                                            // 7
    "function main(): void {\n"                      // 8
    "    Foo f = new Foo();\n"                       // 9
    "    f.bar();\n"                                 // 10 ('bar' at col 6)
    "    int h = helper();\n"                        // 11 ('helper' at col 12)
    "}\n"                                            // 12
    "main();\n";                                     // 13

} // namespace

void AstPositionIndexTestSuite::registerTests(LspTestHarness& harness) {

    // === walkAst ===

    harness.addTest("walkAst visits nodes of a parsed document", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr, "document should exist");

        int visited = 0;
        walkAst(doc->ast, [&](const ast::ASTNode*) {
            ++visited;
            return false;
        });
        require(visited > 5, "expected the walk to reach multiple nodes, got "
            + std::to_string(visited));
    });

    harness.addTest("walkAst stops as soon as visit returns true", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr, "document should exist");

        int visited = 0;
        bool stopped = walkAst(doc->ast, [&](const ast::ASTNode*) {
            ++visited;
            return true;
        });
        require(stopped, "walkAst should report the early stop");
        require(visited == 1, "expected exactly 1 visit, got "
            + std::to_string(visited));
    });

    // === findMethodCallAt ===

    harness.addTest("findMethodCallAt hits the call anchored at the cursor", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr, "document should exist");

        const auto* call = findMethodCallAt(doc->ast, 10, 6, "bar");
        require(call != nullptr, "expected MethodCallNode for f.bar() at 10:6");
        require(call->getMethodName() == "bar",
            "expected method name 'bar', got '" + call->getMethodName() + "'");
    });

    harness.addTest("findMethodCallAt rejects a non-matching name", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr, "document should exist");

        require(findMethodCallAt(doc->ast, 10, 6, "baz") == nullptr,
            "name mismatch should return null");
    });

    harness.addTest("findMethodCallAt rejects the wrong line", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr, "document should exist");

        require(findMethodCallAt(doc->ast, 2, 6, "bar") == nullptr,
            "wrong line should return null");
    });

    // === findFunctionCallAt ===

    harness.addTest("findFunctionCallAt hits a free function call", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr, "document should exist");

        // FunctionCallNode is anchored after its argument list (not at the
        // name token like MethodCallNode), so findFunctionCallAt matches by
        // line + name; the column argument cannot discriminate.
        const auto* call = findFunctionCallAt(doc->ast, 11, 12, "helper");
        require(call != nullptr, "expected FunctionCallNode for helper() on line 11");

        require(findFunctionCallAt(doc->ast, 5, 12, "helper") == nullptr,
            "wrong line should return null");
        require(findFunctionCallAt(doc->ast, 11, 12, "nope") == nullptr,
            "name mismatch should return null");
    });

    // === findEnclosingCallable ===

    harness.addTest("findEnclosingCallable finds the function around the cursor", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr, "document should exist");

        const auto* callable = findEnclosingCallable(doc->ast, 10, 4);
        require(callable != nullptr,
            "cursor inside main's body should have an enclosing callable");
    });

    harness.addTest("findEnclosingCallable returns null before any callable", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr, "document should exist");

        // Matching is by source-anchor line only (no body end ranges), so
        // "null" is only reachable when the cursor precedes every callable
        // anchor. Line 0 is the class declaration — bar's anchor is line 1.
        require(findEnclosingCallable(doc->ast, 0, 0) == nullptr,
            "cursor before any callable anchor should resolve to null");
    });

    harness.addTest("findEnclosingCallable after all bodies picks the nearest preceding one", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr, "document should exist");

        // BY DESIGN best-effort pin: anchors carry no end ranges, so a
        // top-level cursor after main's body (line 13, `main();`) resolves
        // to the nearest preceding callable rather than null.
        require(findEnclosingCallable(doc->ast, 13, 0) != nullptr,
            "anchor-only matching resolves trailing top-level lines to the last callable");
    });

    // === findEnclosingClass ===

    harness.addTest("findEnclosingClass finds the class around the cursor", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr, "document should exist");

        const auto* cls = findEnclosingClass(doc->ast, 2, 8);
        require(cls != nullptr, "cursor inside Foo should find the class");
        require(cls->getClassName() == "Foo",
            "expected class 'Foo', got '" + cls->getClassName() + "'");
    });

    harness.addTest("findEnclosingClass returns null in a classless document", []() {
        // Matching is by source-anchor line only, so once a class anchor
        // precedes the cursor it always wins; null is only reachable when
        // no class anchor precedes the cursor at all.
        auto docMgr = makeDocManager("file:///noclass.mt",
            "function lone(): void {\n"
            "    print(1);\n"
            "}\n"
            "lone();\n");
        auto* doc = docMgr->getDocument("file:///noclass.mt");
        require(doc != nullptr, "document should exist");

        require(findEnclosingClass(doc->ast, 1, 4) == nullptr,
            "classless document should have no enclosing class");
    });
}

} // namespace mtype::lsp::test
