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
    "function helper(): void {}\n"                   // 5
    "function main(): void {\n"                      // 6
    "    Foo f = new Foo();\n"                       // 7
    "    f.bar();\n"                                 // 8  ('bar' at col 6)
    "    helper();\n"                                // 9  ('helper' at col 4)
    "}\n"                                            // 10
    "main();\n";                                     // 11

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

        const auto* call = findMethodCallAt(doc->ast, 8, 6, "bar");
        require(call != nullptr, "expected MethodCallNode for f.bar() at 8:6");
        require(call->getMethodName() == "bar",
            "expected method name 'bar', got '" + call->getMethodName() + "'");
    });

    harness.addTest("findMethodCallAt rejects a non-matching name", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr, "document should exist");

        require(findMethodCallAt(doc->ast, 8, 6, "baz") == nullptr,
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

        const auto* call = findFunctionCallAt(doc->ast, 9, 4, "helper");
        require(call != nullptr, "expected FunctionCallNode for helper() at 9:4");
    });

    // === findEnclosingCallable ===

    harness.addTest("findEnclosingCallable finds the function around the cursor", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr, "document should exist");

        const auto* callable = findEnclosingCallable(doc->ast, 8, 4);
        require(callable != nullptr,
            "cursor inside main's body should have an enclosing callable");
    });

    harness.addTest("findEnclosingCallable returns null at top level", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr, "document should exist");

        // Line 11 is the top-level `main();` statement.
        require(findEnclosingCallable(doc->ast, 11, 0) == nullptr,
            "top-level cursor should have no enclosing callable");
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

    harness.addTest("findEnclosingClass returns null outside any class", []() {
        auto docMgr = makeDocManager("file:///t.mt", kSource);
        auto* doc = docMgr->getDocument("file:///t.mt");
        require(doc != nullptr, "document should exist");

        require(findEnclosingClass(doc->ast, 9, 4) == nullptr,
            "cursor inside a free function should find no enclosing class");
    });
}

} // namespace mtype::lsp::test
