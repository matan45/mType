#include "ControlFlowCompiler.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/ParseException.hpp"

namespace vm::compiler::visitors
{
    ControlFlowCompiler::ControlFlowCompiler(CompilerContext& context)
        : ctx(context)
    {
    }

    value::Value ControlFlowCompiler::compileIf(ast::IfNode* node)
    {
        // Compile condition
        node->getCondition()->accept(ctx.visitor);  // Will need delegation

        // Jump to else/end if condition is false
        size_t elseJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Compile then branch
        node->getThenStatement()->accept(ctx.visitor);  // Will need delegation

        if (node->getElseStatement()) {
            // Jump over else branch
            size_t endJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

            // Patch else jump
            ctx.emitter.patchJump(elseJump);

            // Compile else branch
            node->getElseStatement()->accept(ctx.visitor);  // Will need delegation

            // Patch end jump
            ctx.emitter.patchJump(endJump);
        } else {
            // No else branch, patch jump to end
            ctx.emitter.patchJump(elseJump);
        }

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileWhile(ast::WhileNode* node)
    {
        size_t loopStart = ctx.program.getCurrentOffset();

        // Compile condition
        node->getCondition()->accept(ctx.visitor);  // Will need delegation

        // Jump to end if condition is false
        size_t exitJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Enter loop context
        ctx.loopManager.enterLoop(loopStart);

        // Compile body
        node->getBody()->accept(ctx.visitor);  // Will need delegation

        // Jump back to start
        ctx.emitter.emitLoop(loopStart);

        // Exit loop and patch jumps
        ctx.emitter.patchJump(exitJump);
        for (size_t breakJump : ctx.loopManager.getBreakJumps()) {
            ctx.emitter.patchJump(breakJump);
        }
        for (size_t continueJump : ctx.loopManager.getContinueJumps()) {
            ctx.program.patchJump(continueJump, static_cast<uint32_t>(loopStart));
        }
        ctx.loopManager.exitLoop();

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileDoWhile(ast::DoWhileNode* node)
    {
        size_t loopStart = ctx.program.getCurrentOffset();

        // Enter loop context
        ctx.loopManager.enterLoop(loopStart);

        // Compile body
        node->getBody()->accept(ctx.visitor);  // Will need delegation

        // Compile condition
        node->getCondition()->accept(ctx.visitor);  // Will need delegation

        // Jump back to start if condition is true
        size_t continueJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_TRUE);
        ctx.program.patchJump(continueJump, static_cast<uint32_t>(loopStart));

        // Exit loop and patch jumps
        for (size_t breakJump : ctx.loopManager.getBreakJumps()) {
            ctx.emitter.patchJump(breakJump);
        }
        for (size_t contJump : ctx.loopManager.getContinueJumps()) {
            ctx.program.patchJump(contJump, static_cast<uint32_t>(loopStart));
        }
        ctx.loopManager.exitLoop();

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileFor(ast::ForNode* node)
    {
        ctx.variableTracker.beginScope();

        // Compile initializer
        if (node->getInitialization()) {
            node->getInitialization()->accept(ctx.visitor);  // Will need delegation
        }

        size_t loopStart = ctx.program.getCurrentOffset();

        // Compile condition
        if (node->getCondition()) {
            node->getCondition()->accept(ctx.visitor);  // Will need delegation
        } else {
            // No condition means infinite loop
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_BOOL, 1, node);
        }

        size_t exitJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Jump over increment to body
        size_t bodyJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Increment location (continue jumps here)
        size_t incrementStart = ctx.program.getCurrentOffset();
        if (node->getUpdate()) {
            node->getUpdate()->accept(ctx.visitor);  // Will need delegation
            ctx.program.emit(bytecode::OpCode::POP);  // Discard increment result
        }

        // Jump back to condition
        ctx.emitter.emitLoop(loopStart);

        // Body starts here
        ctx.emitter.patchJump(bodyJump);

        // Enter loop context
        ctx.loopManager.enterLoop(loopStart, incrementStart);

        // Compile body
        if (node->getBody()) {
            node->getBody()->accept(ctx.visitor);  // Will need delegation
        }

        // Jump to increment
        ctx.emitter.emitLoop(incrementStart);

        // Exit loop and patch jumps
        ctx.emitter.patchJump(exitJump);
        for (size_t breakJump : ctx.loopManager.getBreakJumps()) {
            ctx.emitter.patchJump(breakJump);
        }
        for (size_t continueJump : ctx.loopManager.getContinueJumps()) {
            ctx.program.patchJump(continueJump, static_cast<uint32_t>(incrementStart));
        }
        ctx.loopManager.exitLoop();

        ctx.variableTracker.endScope();
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileForEach(ast::ForEachNode* node)
    {
        std::string varName = node->getVariableName();
        value::ValueType varType = node->getVariableType();

        ctx.variableTracker.beginScope();

        // Evaluate collection
        node->getCollection()->accept(ctx.visitor);  // Will need delegation

        // Store collection in local
        ctx.variableTracker.declareLocal("__foreach_array__", value::ValueType::OBJECT, "");
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
        size_t arraySlot = ctx.variableTracker.getNextLocalSlot() - 1;
        ctx.program.emit(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(arraySlot));

        // Get array length with source location
        ctx.program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(arraySlot));
        ctx.program.emit(bytecode::OpCode::ARRAY_LENGTH);
        ctx.program.addSourceLocation(ctx.program.getCurrentOffset() - 1,
                                     node->getLocation().getLine(),
                                     node->getLocation().getColumn(),
                                     node->getLocation().getFilename());

        // Store length in local
        ctx.variableTracker.declareLocal("__foreach_length__", value::ValueType::INT, "");
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
        size_t lengthSlot = ctx.variableTracker.getNextLocalSlot() - 1;
        ctx.program.emit(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(lengthSlot));

        // Initialize counter
        ctx.program.emit(bytecode::OpCode::PUSH_INT,
                       static_cast<uint32_t>(ctx.program.getConstantPool().addInteger(0)));
        ctx.variableTracker.declareLocal("__foreach_counter__", value::ValueType::INT, "");
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
        size_t counterSlot = ctx.variableTracker.getNextLocalSlot() - 1;
        ctx.program.emit(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(counterSlot));

        // Loop start
        size_t loopStart = ctx.program.getCurrentOffset();
        ctx.program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(counterSlot));
        ctx.program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(lengthSlot));
        ctx.program.emit(bytecode::OpCode::LT);

        size_t exitJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Get current element with source location
        ctx.program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(arraySlot));
        ctx.program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(counterSlot));
        ctx.program.emit(bytecode::OpCode::ARRAY_GET);
        ctx.program.addSourceLocation(ctx.program.getCurrentOffset() - 1,
                                     node->getLocation().getLine(),
                                     node->getLocation().getColumn(),
                                     node->getLocation().getFilename());

        // Store in loop variable
        ctx.variableTracker.declareLocal(varName, varType, "");
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
        size_t loopVarSlot = ctx.variableTracker.getNextLocalSlot() - 1;
        ctx.program.emit(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(loopVarSlot));

        // Enter loop context
        size_t continueTarget = ctx.program.getCurrentOffset();
        ctx.loopManager.enterLoop(loopStart, continueTarget);

        // Compile body
        if (node->getBody()) {
            node->getBody()->accept(ctx.visitor);  // Will need delegation
        }

        ctx.loopManager.exitLoop();

        // Increment counter
        ctx.program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(counterSlot));
        ctx.program.emit(bytecode::OpCode::PUSH_INT,
                       static_cast<uint32_t>(ctx.program.getConstantPool().addInteger(1)));
        ctx.program.emit(bytecode::OpCode::ADD);
        ctx.program.emit(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(counterSlot));
        ctx.program.emit(bytecode::OpCode::POP);

        // Jump back to loop start
        ctx.emitter.emitLoop(loopStart);

        // Patch exit jump
        ctx.emitter.patchJump(exitJump);

        ctx.variableTracker.endScope();

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileSwitch(ast::SwitchNode* node)
    {
        ctx.switchManager.enterSwitch();

        // Compile switch expression
        node->getExpression()->accept(ctx.visitor);  // Will need delegation

        const auto& cases = node->getCases();
        std::vector<size_t> caseBodyStarts;
        size_t defaultBodyStart = SIZE_MAX;
        size_t defaultCaseIndex = SIZE_MAX;

        // First pass: generate comparisons
        for (size_t i = 0; i < cases.size(); ++i) {
            if (dynamic_cast<ast::DefaultCaseNode*>(cases[i].get())) {
                defaultCaseIndex = i;
                caseBodyStarts.push_back(SIZE_MAX);
            } else if (auto* regularCase = dynamic_cast<ast::CaseNode*>(cases[i].get())) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::DUP, regularCase);
                regularCase->getValue()->accept(ctx.visitor);  // Will need delegation
                ctx.emitter.emitWithLocation(bytecode::OpCode::EQ, regularCase);
                size_t jumpToCaseBody = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_TRUE);
                caseBodyStarts.push_back(jumpToCaseBody);
            }
        }

        size_t jumpToDefaultOrEnd = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Second pass: compile case bodies
        for (size_t i = 0; i < cases.size(); ++i) {
            if (caseBodyStarts[i] != SIZE_MAX) {
                ctx.emitter.patchJump(caseBodyStarts[i]);
            }

            if (i == defaultCaseIndex) {
                defaultBodyStart = ctx.program.getCurrentOffset();
            }

            if (caseBodyStarts[i] != SIZE_MAX || i == defaultCaseIndex) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::POP, cases[i].get());
            }

            // Compile case statements
            if (auto* defaultCase = dynamic_cast<ast::DefaultCaseNode*>(cases[i].get())) {
                for (const auto& stmt : defaultCase->getStatements()) {
                    stmt->accept(ctx.visitor);  // Will need delegation
                }
            } else if (auto* regularCase = dynamic_cast<ast::CaseNode*>(cases[i].get())) {
                for (const auto& stmt : regularCase->getStatements()) {
                    stmt->accept(ctx.visitor);  // Will need delegation
                }
            }
        }

        size_t implicitEndJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Patch jump to default or end
        ctx.emitter.patchJump(jumpToDefaultOrEnd);
        if (defaultBodyStart != SIZE_MAX) {
            ctx.program.emit(bytecode::OpCode::JUMP, static_cast<uint32_t>(defaultBodyStart));
        } else {
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);
        }

        // Patch all break jumps
        ctx.emitter.patchJump(implicitEndJump);
        for (size_t breakJump : ctx.switchManager.getBreakJumps()) {
            ctx.emitter.patchJump(breakJump);
        }

        ctx.switchManager.exitSwitch();

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileCase(ast::CaseNode* node)
    {
        // Case nodes are handled by SwitchNode
        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileDefaultCase(ast::DefaultCaseNode* node)
    {
        // Default case nodes are handled by SwitchNode
        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileBreak(ast::BreakNode* node)
    {
        // IMPORTANT: Check switch context first!
        // When a switch is nested in a loop, break should exit the switch, not the loop
        if (ctx.switchManager.isInSwitch()) {
            size_t breakJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
            ctx.switchManager.registerBreak(breakJump);
        } else if (ctx.loopManager.isInLoop()) {
            size_t breakJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
            ctx.loopManager.registerBreak(breakJump);
        } else {
            throw errors::ParseException("Break outside of loop or switch");
        }
        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileContinue(ast::ContinueNode* node)
    {
        if (!ctx.loopManager.isInLoop()) {
            throw errors::ParseException("Continue outside of loop");
        }

        size_t continueJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
        ctx.loopManager.registerContinue(continueJump);
        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileTry(ast::TryNode* node)
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
        const auto& catchBlocks = node->getCatchBlocks();
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

        // Compile finally block if present
        if (node->getFinallyBlock()) {
            size_t finallyOffset = ctx.program.getCurrentOffset();
            ctx.exceptionManager.setFinallyOffset(finallyOffset);

            // Patch RETURN jumps to point to the finally block
            // These are from return statements and will fall through to RETURN_VALUE
            for (size_t returnJump : ctx.exceptionManager.getReturnJumps()) {
                ctx.emitter.patchJump(returnJump);
            }

            // Emit FINALLY instruction
            ctx.program.emit(bytecode::OpCode::FINALLY);

            // Mark that we're now in the finally block
            // This prevents return statements inside finally from trying to jump to finally again
            ctx.exceptionManager.enterFinally();

            // Compile finally body
            node->getFinallyBlock()->accept(ctx.visitor);

            // Mark that we've exited the finally block
            ctx.exceptionManager.exitFinally();

            // After finally, load the return value back from the special slot if it was saved
            size_t returnValueSlot = ctx.exceptionManager.getReturnValueSlot();
            if (returnValueSlot != SIZE_MAX) {
                ctx.program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(returnValueSlot));
            }

            // Check if we're inside another try block with a finally (nested try-finally)
            bool hasOuterFinally = ctx.exceptionManager.hasOuterFinally();

            if (hasOuterFinally) {
                // There's an outer finally - we need to:
                // 1. Store the return value in the outer context's slot (if not already set)
                // 2. Jump to the outer finally

                // Check if outer already has a return value slot allocated
                size_t outerReturnValueSlot = ctx.exceptionManager.getReturnValueSlotForOuter();
                if (outerReturnValueSlot == SIZE_MAX) {
                    // Outer doesn't have a slot yet - allocate one for it
                    outerReturnValueSlot = ctx.variableTracker.getNextLocalSlot();
                    ctx.functionFrameManager.updateMaxLocalSlot(outerReturnValueSlot + 1);
                    ctx.exceptionManager.setReturnValueSlotForOuter(outerReturnValueSlot);
                }

                // Store return value in outer's slot (return value is on stack from LOAD_LOCAL above)
                ctx.program.emit(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(outerReturnValueSlot));

                // Jump to the outer finally
                size_t jumpToOuterFinally = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
                // Register this jump with the OUTER context so it gets patched to the outer finally
                ctx.exceptionManager.registerReturnJumpWithOuter(jumpToOuterFinally);
            } else {
                // No outer finally - emit RETURN_VALUE to actually return
                ctx.program.emit(bytecode::OpCode::RETURN_VALUE);
            }

            // Now patch normal EXIT jumps to jump PAST the RETURN_VALUE/JUMP
            // These are from normal try/catch completion (no return)
            size_t afterReturn = ctx.program.getCurrentOffset();
            for (size_t exitJump : ctx.exceptionManager.getExitJumps()) {
                ctx.program.patchJump(exitJump, static_cast<uint32_t>(afterReturn));
            }
        } else {
            // No finally block - patch all jumps to point here (end of try-catch)
            for (size_t exitJump : ctx.exceptionManager.getExitJumps()) {
                ctx.emitter.patchJump(exitJump);
            }
            for (size_t returnJump : ctx.exceptionManager.getReturnJumps()) {
                ctx.emitter.patchJump(returnJump);
            }
        }

        // Exit exception context
        ctx.exceptionManager.exitTry();

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileThrow(ast::ThrowNode* node)
    {
        // Compile the exception expression (should evaluate to an object)
        node->getException()->accept(ctx.visitor);

        // Emit THROW instruction with source location
        ctx.emitter.emitWithLocation(bytecode::OpCode::THROW, node);

        return std::monostate{};
    }
}
