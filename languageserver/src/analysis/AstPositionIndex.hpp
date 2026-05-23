#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ast { class ASTNode; }
namespace ast::nodes::classes { class MethodCallNode; }
namespace ast::nodes::functions { class FunctionCallNode; class FunctionNode; }

// Namespace stays `mtype::lsp` (not `mtype::lsp::analysis`) so that the
// engine-side `::analysis::OverrideAnnotationChecker` / `UnusedVariableAnalyzer`
// references in DocumentManager.cpp resolve to the root namespace and don't
// get shadowed by an inner one.
namespace mtype::lsp {

// MYT-359 — light-weight AST traversal helpers used by ReceiverTypeResolver.
//
// AST nodes only carry a single (line, column) point (the parse anchor) — not
// a source range — so matching "the node at the cursor" is by anchor, not
// envelope. For MethodCallNode and FunctionCallNode the parser anchors the
// node at the method/function-name token (see ExpressionParser.cpp:591), so
// matching against the cursor's identifier-span column-window is reliable
// when the cursor is on the call name (which is exactly when the LSP
// invokes go-to-def / hover for a call).

// Calls `visit(node)` for every node reachable from `node`. If `visit`
// returns true, traversal stops immediately and walkAst returns true.
bool walkAst(const ast::ASTNode* node,
             const std::function<bool(const ast::ASTNode*)>& visit);

bool walkAst(const std::vector<std::unique_ptr<ast::ASTNode>>& roots,
             const std::function<bool(const ast::ASTNode*)>& visit);

// Finds the MethodCallNode whose method-name token sits at LSP (line, col)
// (both 0-based) and whose method name matches `methodName`. Returns null
// when no such call is found. Caller is expected to treat the returned
// pointer as a borrow with lifetime tied to the underlying document AST.
const ast::nodes::classes::MethodCallNode* findMethodCallAt(
    const std::vector<std::unique_ptr<ast::ASTNode>>& roots,
    int line, int col,
    const std::string& methodName);

// Same shape as findMethodCallAt for free function calls.
const ast::nodes::functions::FunctionCallNode* findFunctionCallAt(
    const std::vector<std::unique_ptr<ast::ASTNode>>& roots,
    int line, int col,
    const std::string& functionName);

// Finds the enclosing FunctionNode / MethodNode / ConstructorNode whose
// body contains the LSP cursor. Used by the receiver resolver to pull
// parameter types for a VariableNode that names a parameter. Returns the
// enclosing AST node as a base ASTNode* — callers dispatch via dynamic_cast.
// Returns null if the cursor is at top-level (outside any function body).
const ast::ASTNode* findEnclosingCallable(
    const std::vector<std::unique_ptr<ast::ASTNode>>& roots,
    int line, int col);

}  // namespace mtype::lsp
