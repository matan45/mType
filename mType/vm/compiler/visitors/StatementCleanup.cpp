#include "StatementCleanup.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../ast/nodes/classes/MethodCallNode.hpp"

namespace vm::compiler::visitors::statementCleanup
{
    bool isExpressionStatement(ast::ASTNode* node)
    {
        using namespace ast::nodes::functions;

        // Top-level calls leave exactly one return value on the operand stack:
        //   - FunctionCallNode: native calls always push; bytecode calls push
        //     via RETURN_VALUE.
        //   - MethodCallNode (MYT-152): non-void methods push via RETURN_VALUE;
        //     void methods push PUSH_NULL + RETURN_VALUE (see
        //     MethodCompilerHelper::compileMethodBodyWithFrame). Async-void is
        //     identical with a CREATE_PROMISE before RETURN_VALUE. In all
        //     cases one slot must be discarded at statement position.
        if (dynamic_cast<FunctionCallNode*>(node)) return true;
        if (dynamic_cast<ast::nodes::classes::MethodCallNode*>(node)) return true;

        return false;
    }

    void emitStatementCleanup(CompilerContext& ctx, ast::ASTNode* stmt, std::size_t offsetBefore)
    {
        // Path A: flagged expression statements (top-level function calls).
        if (isExpressionStatement(stmt))
        {
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, stmt);
            return;
        }

        // Path B: stores that re-push the stored value at runtime
        // (VariableExecutor::handleStoreLocal, handleStoreVar and all its
        // helpers, ObjectExecutor::handleSetField) do so to support
        // expression-context assignments like `int x = (a = 5)` — in
        // statement context nothing consumes that re-pushed value, so the
        // operand stack grows by 1 per statement and OSR can't tier up any
        // loop containing such a statement. SET_STATIC, ARRAY_SET, and
        // INLINE_SET_FIELD have pure-consume semantics, so they don't need a
        // POP.
        std::size_t offsetAfter = ctx.program.getCurrentOffset();
        if (offsetAfter > offsetBefore)
        {
            const auto& lastInstr = ctx.program.getInstruction(offsetAfter - 1);
            switch (lastInstr.opcode)
            {
                case bytecode::OpCode::STORE_LOCAL:
                case bytecode::OpCode::STORE_VAR:
                case bytecode::OpCode::SET_FIELD:
                    ctx.emitter.emitWithLocation(bytecode::OpCode::POP, stmt);
                    break;
                default:
                    break;
            }
        }
    }
}
