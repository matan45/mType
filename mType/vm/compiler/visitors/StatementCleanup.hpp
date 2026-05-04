#pragma once
#include "CompilerContext.hpp"
#include "../../../ast/ASTNode.hpp"
#include <cstddef>

namespace vm::compiler::visitors::statementCleanup
{
    // Returns true if `node` is a top-level call expression used as a
    // statement (FunctionCallNode or MethodCallNode). Such calls leave their
    // return value on the operand stack and require a trailing POP at
    // statement position.
    bool isExpressionStatement(ast::ASTNode* node);

    // Emit the trailing POP needed to keep the operand stack clean after a
    // statement, covering:
    //   (A) expression statements flagged by isExpressionStatement, and
    //   (B) statements whose last emitted opcode is STORE_LOCAL / STORE_VAR /
    //       SET_FIELD — those instructions re-push the stored value at
    //       runtime to support cascading expressions like `int x = (a = 5)`,
    //       and in statement context nothing consumes the re-pushed value.
    //
    // `offsetBefore` is `ctx.program.getCurrentOffset()` captured BEFORE the
    // statement's accept() call. When the statement is a BlockNode the helper
    // is a no-op: BlockNode is not an expression-statement, and compileBlock
    // already drained any trailing STORE_LOCAL inside the block.
    //
    // MYT-271: shared between StatementCompiler::compileBlock (braced bodies)
    // and ControlFlowCompiler (braceless if/while/do-while/for/forEach
    // bodies). Without this helper at the braceless sites, `if (cond) x = ...`
    // and `if (cond) fn();` leak operand-stack slots.
    void emitStatementCleanup(CompilerContext& ctx, ast::ASTNode* stmt, std::size_t offsetBefore);
}
