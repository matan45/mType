#include "TryStatementHelper.hpp"
#include "../../bytecode/OpCode.hpp"

namespace vm::compiler::visitors
{
    TryStatementHelper::TryStatementHelper(CompilerContext& context)
        : ctx(context)
    {
    }

    value::Value TryStatementHelper::compileTry(ast::TryNode* node)
    {
        // Emit TRY_BEGIN instruction
        size_t tryBeginOffset = ctx.program.getCurrentOffset();
        ctx.program.emit(bytecode::OpCode::TRY_BEGIN);

        // Enter exception context (tell it if there's a finally block)
        bool hasFinally = (node->getFinallyBlock() != nullptr);
        ctx.exceptionManager.enterTry(tryBeginOffset, hasFinally);

        // Compile try block
        node->getTryBlock()->accept(ctx.visitor);

        // Mark end of try block
        size_t tryEndOffset = ctx.program.getCurrentOffset();
        ctx.exceptionManager.setTryEndOffset(tryEndOffset);

        // Jump over catch blocks if no exception thrown
        size_t endJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
        ctx.exceptionManager.registerExitJump(endJump);

        // Emit TRY_END instruction
        ctx.program.emit(bytecode::OpCode::TRY_END);

        // Compile catch blocks
        compileCatchBlocks(node->getCatchBlocks());

        // Compile finally block if present
        compileFinallyBlock(dynamic_cast<ast::BlockNode*>(node->getFinallyBlock()));

        // Exit exception context
        ctx.exceptionManager.exitTry();

        return std::monostate{};
    }

    void TryStatementHelper::compileCatchBlocks(const std::vector<std::unique_ptr<ast::CatchNode>>& catchBlocks)
    {
        for (const auto& catchBlock : catchBlocks) {
            size_t catchHandlerOffset = ctx.program.getCurrentOffset();

            // Register catch handler
            ctx.exceptionManager.registerCatchHandler(
                catchBlock->getExceptionType(),
                catchBlock->getVariableName(),
                catchHandlerOffset
            );

            // Emit CATCH instruction with exception type
            std::string exceptionType = catchBlock->getExceptionType();
            uint32_t typeIndex = static_cast<uint32_t>(ctx.program.getConstantPool().addString(exceptionType));
            ctx.program.emit(bytecode::OpCode::CATCH, typeIndex);

            // Enter scope for catch variable
            ctx.variableTracker.beginScope();

            // Declare catch variable (exception is on stack)
            // Note: Only declare in variableTracker, NOT in globalRegistry
            // This ensures it's treated as a local variable (LOAD_LOCAL) not global (LOAD_VAR)
            ctx.variableTracker.declareLocal(
                catchBlock->getVariableName(),
                value::ValueType::OBJECT,
                catchBlock->getExceptionType()
            );

            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t catchVarSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.program.emit(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(catchVarSlot));

            // Compile catch body
            catchBlock->getBody()->accept(ctx.visitor);

            // Exit catch scope
            ctx.variableTracker.endScope();
            ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

            // Jump to end after catch block
            size_t catchEndJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
            ctx.exceptionManager.registerExitJump(catchEndJump);
        }
    }

    void TryStatementHelper::compileFinallyBlock(ast::BlockNode* finallyBlock)
    {
        if (!finallyBlock) {
            // No finally block - patch all jumps to point here (end of try-catch)
            for (size_t exitJump : ctx.exceptionManager.getExitJumps()) {
                ctx.emitter.patchJump(exitJump);
            }
            for (size_t returnJump : ctx.exceptionManager.getReturnJumps()) {
                ctx.emitter.patchJump(returnJump);
            }
            return;
        }

        size_t finallyOffset = ctx.program.getCurrentOffset();
        ctx.exceptionManager.setFinallyOffset(finallyOffset);

        // Patch BOTH exit jumps AND return jumps to point to the finally block
        // Finally must ALWAYS execute, whether we're returning or completing normally
        for (size_t exitJump : ctx.exceptionManager.getExitJumps()) {
            ctx.emitter.patchJump(exitJump);
        }
        for (size_t returnJump : ctx.exceptionManager.getReturnJumps()) {
            ctx.emitter.patchJump(returnJump);
        }

        // Emit FINALLY instruction
        ctx.program.emit(bytecode::OpCode::FINALLY);

        // Mark that we're now in the finally block
        ctx.exceptionManager.enterFinally();

        // Compile finally body
        finallyBlock->accept(ctx.visitor);

        // Mark that we've exited the finally block
        ctx.exceptionManager.exitFinally();

        // After finally completes normally, handle return logic if there was a return statement
        size_t returnValueSlot = ctx.exceptionManager.getReturnValueSlot();
        if (returnValueSlot != SIZE_MAX) {
            // Load the return value back from the special slot
            ctx.program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(returnValueSlot));

            // Check if we're inside another try block with a finally (nested try-finally)
            bool hasOuterFinally = ctx.exceptionManager.hasOuterFinally();

            if (hasOuterFinally) {
                // There's an outer finally - store return value and jump to it
                size_t outerReturnValueSlot = ctx.exceptionManager.getReturnValueSlotForOuter();
                if (outerReturnValueSlot == SIZE_MAX) {
                    outerReturnValueSlot = ctx.variableTracker.getNextLocalSlot();
                    ctx.functionFrameManager.updateMaxLocalSlot(outerReturnValueSlot + 1);
                    ctx.exceptionManager.setReturnValueSlotForOuter(outerReturnValueSlot);
                }

                ctx.program.emit(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(outerReturnValueSlot));

                size_t jumpToOuterFinally = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
                ctx.exceptionManager.registerReturnJumpWithOuter(jumpToOuterFinally);
            } else {
                // No outer finally - emit RETURN_VALUE to actually return
                ctx.program.emit(bytecode::OpCode::RETURN_VALUE);
            }
        }

        // If we reach here, finally completed normally with no return
        // Just fall through to continue execution after the try-catch-finally
    }
}
