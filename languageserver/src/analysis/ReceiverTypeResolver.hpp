#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../../../mType/types/UnifiedType.hpp"

namespace ast { class ASTNode; }
namespace ast::nodes::expressions {
    class VariableNode;
    class IndexAccessNode;
    class TernaryExpNode;
}
namespace ast::nodes::classes {
    class MemberAccessNode;
    class MethodCallNode;
    class NewNode;
}
namespace ast::nodes::functions { class FunctionCallNode; }
namespace environment { class Environment; }

// Namespace stays `mtype::lsp` (not `mtype::lsp::analysis`) so the engine-side
// `::analysis::*` references elsewhere in the LSP code don't get shadowed.
namespace mtype::lsp {

// MYT-359 — AST-driven receiver-type inference for LSP go-to-definition and
// hover. Replaces the regex-based DocumentManager::inferVariableType(), which
// only saw `Type name = ...` / `Type name = new Type(...)` / `for (Type name :
// ...)` and silently no-op'd on `obj.field.method()`, `getFoo().method()`,
// `arr[0].method()`, `Class::field.field.method()`, `(cond ? a : b).method()`.
//
// Operates as a pure function over the document AST + Environment:
//   - Pulls field types from FieldDefinition::getUnifiedType()        (MYT-357)
//   - Pulls method return types from MethodDefinition::getUnifiedReturnType()
//   - Pulls free-function return types from FunctionDefinition::getUnifiedReturnType()  (MYT-359, this ticket)
//   - Falls back to walking the document AST for local-variable / parameter
//     declarations the LSP environment doesn't track per-scope.
//
// Returns nullptr when the receiver shape isn't handled (caller falls back to
// the existing direct-symbol lookup paths in DocumentManager).
class ReceiverTypeResolver {
public:
    ReceiverTypeResolver(std::shared_ptr<environment::Environment> env,
                         const std::vector<std::unique_ptr<ast::ASTNode>>* documentAst,
                         int cursorLine = 0,
                         int cursorCol = 0);

    types::UnifiedTypePtr resolve(const ast::ASTNode* receiver) const;

    // Convenience: resolve and return only the base class/interface name
    // (e.g., "Foo" for Foo, "Foo" for Foo[], "List" for List<Foo>). Empty
    // string if resolution fails. Used by DocumentManager call sites that
    // currently key into symbolLocations by class name.
    std::string resolveName(const ast::ASTNode* receiver) const;

    // Look up a bare variable / class-name's declared type. Public so the
    // legacy DocumentManager::getVariableType API (called from
    // MemberCompletionProvider, SignatureHelpHandler, ReferencesHandler) can
    // route through the same resolver without spinning up a synthetic AST.
    types::UnifiedTypePtr lookupVariableType(const std::string& name) const;

private:
    types::UnifiedTypePtr resolveVariable(const ast::nodes::expressions::VariableNode& node) const;
    types::UnifiedTypePtr resolveMemberAccess(const ast::nodes::classes::MemberAccessNode& node) const;
    types::UnifiedTypePtr resolveMethodCall(const ast::nodes::classes::MethodCallNode& node) const;
    types::UnifiedTypePtr resolveFunctionCall(const ast::nodes::functions::FunctionCallNode& node) const;
    types::UnifiedTypePtr resolveIndexAccess(const ast::nodes::expressions::IndexAccessNode& node) const;
    types::UnifiedTypePtr resolveTernary(const ast::nodes::expressions::TernaryExpNode& node) const;
    types::UnifiedTypePtr resolveNew(const ast::nodes::classes::NewNode& node) const;

    // MYT-362 — interface-side fallback for resolveMethodCall. Walks the
    // interface and its `extends` parents, returning the matching method
    // signature's return type. Preferred-match: argCount equals; otherwise
    // first method by name.
    types::UnifiedTypePtr resolveInterfaceMethodReturn(
        const std::string& interfaceName,
        const std::string& methodName,
        size_t argCount) const;

    // Internal helpers for lookupVariableType's fallback chain.
    types::UnifiedTypePtr findLocalDeclType(const std::string& name) const;
    types::UnifiedTypePtr findParamType(const std::string& name) const;

    std::shared_ptr<environment::Environment> env_;
    const std::vector<std::unique_ptr<ast::ASTNode>>* documentAst_;
    int cursorLine_;
    int cursorCol_;
};

// Helper: build a UnifiedType from a className string that may carry a `[]`
// array suffix (e.g., "Foo[]" → arrayOf(classType("Foo"))). Returns nullptr
// for empty input. Exposed for unit tests.
types::UnifiedTypePtr unifiedTypeFromClassName(const std::string& className);

}  // namespace mtype::lsp
