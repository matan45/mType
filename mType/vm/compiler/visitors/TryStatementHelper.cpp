#include "TryStatementHelper.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/TypeException.hpp"
#include  <iostream>
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
        ctx.emitter.emitWithLocation(bytecode::OpCode::TRY_BEGIN, node);

        // Enter exception context (tell it if there's a finally block)
        bool hasFinally = (node->getFinallyBlock() != nullptr);
        ctx.exceptionManager.enterTry(tryBeginOffset, hasFinally);

        // Get nesting level for exception table
        uint32_t nestingLevel = ctx.exceptionManager.getNestingLevel();

        // Enter scope for try block (variables shouldn't leak to catch/finally)
        ctx.variableTracker.beginScope();

        // Compile try block
        node->getTryBlock()->accept(ctx.visitor);

        // Exit try block scope
        ctx.variableTracker.endScope();
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        // Mark end of try block
        size_t tryEndOffset = ctx.program.getCurrentOffset();
        ctx.exceptionManager.setTryEndOffset(tryEndOffset);

        // Jump over catch blocks if no exception thrown
        size_t endJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
        ctx.exceptionManager.registerExitJump(endJump);

        // Emit TRY_END instruction
        ctx.emitter.emitWithLocation(bytecode::OpCode::TRY_END, node);

        // Compile catch blocks and build exception table entries, collecting their IP ranges
        bool noCatchBlocks = node->getCatchBlocks().empty();
        std::vector<CatchBlockRange> catchRanges;
        if (!noCatchBlocks) {
            catchRanges = compileCatchBlocks(node->getCatchBlocks(), tryBeginOffset, tryEndOffset, nestingLevel);
        }

        // Compile finally block if present, passing catch ranges so finally can protect them too
        compileFinallyBlock(dynamic_cast<ast::BlockNode*>(node->getFinallyBlock()), tryBeginOffset, tryEndOffset, nestingLevel, noCatchBlocks, catchRanges);

        // Exit exception context
        ctx.exceptionManager.exitTry();

        return std::monostate{};
    }

    std::vector<TryStatementHelper::CatchBlockRange> TryStatementHelper::compileCatchBlocks(const std::vector<std::unique_ptr<ast::CatchNode>>& catchBlocks,
                                                size_t tryBeginOffset, size_t tryEndOffset, uint32_t nestingLevel)
    {
        std::vector<CatchBlockRange> catchRanges;

        for (const auto& catchBlock : catchBlocks) {
            size_t catchHandlerOffset = ctx.program.getCurrentOffset();

            // Validate that the catch type is Exception or inherits from Exception
            std::string exceptionType = catchBlock->getExceptionType();

            // Extract base class name from generic type (e.g., "ResultException<Int>" -> "ResultException")
            std::string baseClassName = exceptionType;
            size_t genericStart = exceptionType.find('<');
            if (genericStart != std::string::npos)
            {
                baseClassName = exceptionType.substr(0, genericStart);
            }

            auto classRegistry = ctx.environment->getClassRegistry();
            auto exceptionClass = classRegistry->findClass(baseClassName);

            if (!exceptionClass)
            {
                throw errors::TypeException(
                    "Cannot catch undefined type '" + exceptionType + "'. "
                    "Catch type must be a class that exists in the environment.",
                    catchBlock->getLocation()
                );
            }

            // Check if the class inherits from Exception
            bool inheritsFromException = false;
            auto currentClass = exceptionClass;
            while (currentClass)
            {
                if (currentClass->getName() == "Exception")
                {
                    inheritsFromException = true;
                    break;
                }

                // Check parent class
                if (currentClass->hasParentClass())
                {
                    currentClass = classRegistry->findClass(currentClass->getParentClassName());
                }
                else
                {
                    break;
                }
            }

            if (!inheritsFromException)
            {
                throw errors::TypeException(
                    "Cannot catch type '" + exceptionType + "'. "
                    "Catch blocks can only catch Exception or classes that inherit from Exception.",
                    catchBlock->getLocation()
                );
            }

            // Register catch handler
            ctx.exceptionManager.registerCatchHandler(
                exceptionType,
                catchBlock->getVariableName(),
                catchHandlerOffset
            );

            // Emit CATCH instruction with exception type
            uint64_t typeIndex = static_cast<uint64_t>(ctx.program.getConstantPool().addString(exceptionType));
            ctx.emitter.emitWithLocation(bytecode::OpCode::CATCH, typeIndex, catchBlock.get());

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

            // Convert absolute slot to relative slot for STORE_LOCAL emission
            size_t relativeSlot = catchVarSlot;
            if (ctx.functionFrameManager.isInFunction()) {
                size_t startSlot = ctx.functionFrameManager.currentFrame().localStartSlot;
                relativeSlot = catchVarSlot - startSlot;
            }

            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeSlot), catchBlock.get());

            // Build exception table entry for this catch block
            bytecode::ExceptionTableEntry entry(
                tryBeginOffset,                  // startIP
                tryEndOffset,                    // endIP
                catchHandlerOffset,              // catchIP
                SIZE_MAX,                        // finallyIP (will be updated later if there's a finally block)
                exceptionType,                   // exceptionType
                nestingLevel,                    // nestingLevel
                catchBlock->getVariableName(),   // catchVarName
                relativeSlot                     // catchVarSlot (use relative slot)
            );

            // Add entry to appropriate exception table (function or global)
            std::string currentFunctionName = ctx.functionFrameManager.getCurrentFunctionName();
            if (currentFunctionName.empty()) {
                // Global scope
                ctx.program.getGlobalExceptionTable().addEntry(entry);
            } else {
                // Function scope - get function metadata and add to its exception table
                // Note: Function is pre-registered during FunctionRegistrar phase, so it exists
                auto* funcMetadata = const_cast<bytecode::BytecodeProgram::FunctionMetadata*>(
                    ctx.program.getFunction(currentFunctionName)
                );
                if (funcMetadata) {
                    funcMetadata->exceptionTable.addEntry(entry);
                }
            }

            // Track start of catch body for finally exception table entries
            size_t catchBodyStart = ctx.program.getCurrentOffset();

            // Compile catch body
            catchBlock->getBody()->accept(ctx.visitor);

            // Track end of catch body
            size_t catchBodyEnd = ctx.program.getCurrentOffset();
            catchRanges.push_back({catchBodyStart, catchBodyEnd});

            // Exit catch scope
            ctx.variableTracker.endScope();
            ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

            // Jump to end after catch block
            size_t catchEndJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
            ctx.exceptionManager.registerExitJump(catchEndJump);
        }

        return catchRanges;
    }

    void TryStatementHelper::compileFinallyBlock(ast::BlockNode* finallyBlock, size_t tryBeginOffset,
                                                  size_t tryEndOffset, uint32_t nestingLevel, bool noCatchBlocks,
                                                  const std::vector<CatchBlockRange>& catchRanges)
    {
        if (!finallyBlock) {
            // No finally block - patch all jumps to point here (end of try-catch)
            for (size_t exitJump : ctx.exceptionManager.getExitJumps()) {
                ctx.emitter.patchJump(exitJump);
            }
            for (size_t returnJump : ctx.exceptionManager.getReturnJumps()) {
                ctx.emitter.patchJump(returnJump);
            }
            // For break/continue jumps without finally, register them directly with loop manager
            // Only process these if we're actually in a loop context
            if (ctx.loopManager.isInLoop()) {
                for (size_t breakJump : ctx.exceptionManager.getBreakJumps()) {
                    ctx.loopManager.registerBreak(breakJump);
                }
                for (size_t continueJump : ctx.exceptionManager.getContinueJumps()) {
                    ctx.loopManager.registerContinue(continueJump);
                }
            }
            return;
        }

        // Allocate a flag slot to track whether we're on return path (1) or exit path (0)
        // IMPORTANT: Make sure it doesn't conflict with returnValueSlot
        size_t returnValueSlot = ctx.exceptionManager.getReturnValueSlot();

        // Ensure the return value slot is properly reserved if it exists
        // This prevents finally block variables from overwriting the return value
        if (returnValueSlot != SIZE_MAX) {
            // Make sure our next slot is at least past the return value slot
            while (ctx.variableTracker.getNextLocalSlot() <= returnValueSlot) {
                ctx.variableTracker.incrementLocalSlot();
            }
        }

        size_t hasReturnFlagSlot = ctx.variableTracker.getNextLocalSlot();

        // Reserve the flag slot by incrementing the local slot counter
        // This prevents the finally block's local variables from reusing this slot
        ctx.variableTracker.incrementLocalSlot();

        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
        ctx.exceptionManager.setHasReturnFlagSlot(hasReturnFlagSlot);

        // Convert to relative slot for bytecode emission
        size_t relativeFlagSlot = hasReturnFlagSlot;
        if (ctx.functionFrameManager.isInFunction()) {
            size_t startSlot = ctx.functionFrameManager.currentFrame().localStartSlot;
            relativeFlagSlot = hasReturnFlagSlot - startSlot;
        }

        // Add flag values to constant pool
        size_t normalExitIndex = ctx.program.getConstantPool().addInteger(0);   // Normal exit
        size_t returnIndex = ctx.program.getConstantPool().addInteger(1);       // Return
        size_t breakIndex = ctx.program.getConstantPool().addInteger(2);        // Break
        size_t continueIndex = ctx.program.getConstantPool().addInteger(3);     // Continue

        // Create trampolines for exit jumps that set flag = 0 (normal exit)
        std::vector<size_t> newExitJumps;
        for (size_t exitJump : ctx.exceptionManager.getExitJumps()) {
            size_t trampolineOffset = ctx.program.getCurrentOffset();
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT, static_cast<uint64_t>(normalExitIndex), finallyBlock);
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeFlagSlot), finallyBlock);
            size_t jumpToFinally = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

            // Patch original jump to trampoline
            ctx.program.patchJump(exitJump, static_cast<uint64_t>(trampolineOffset));
            newExitJumps.push_back(jumpToFinally);
        }

        // Create trampolines for return jumps that set flag = 1 (return)
        std::vector<size_t> newReturnJumps;
        for (size_t returnJump : ctx.exceptionManager.getReturnJumps()) {
            size_t trampolineOffset = ctx.program.getCurrentOffset();
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT, static_cast<uint64_t>(returnIndex), finallyBlock);
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeFlagSlot), finallyBlock);
            size_t jumpToFinally = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

            // Patch original jump to trampoline
            ctx.program.patchJump(returnJump, static_cast<uint64_t>(trampolineOffset));
            newReturnJumps.push_back(jumpToFinally);
        }

        // Create trampolines for break jumps that set flag = 2 (break)
        std::vector<size_t> newBreakJumps;
        for (size_t breakJump : ctx.exceptionManager.getBreakJumps()) {
            size_t trampolineOffset = ctx.program.getCurrentOffset();
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT, static_cast<uint64_t>(breakIndex), finallyBlock);
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeFlagSlot), finallyBlock);
            size_t jumpToFinally = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

            // Patch original jump to trampoline
            ctx.program.patchJump(breakJump, static_cast<uint64_t>(trampolineOffset));
            newBreakJumps.push_back(jumpToFinally);
        }

        // Create trampolines for continue jumps that set flag = 3 (continue)
        std::vector<size_t> newContinueJumps;
        for (size_t continueJump : ctx.exceptionManager.getContinueJumps()) {
            size_t trampolineOffset = ctx.program.getCurrentOffset();
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT, static_cast<uint64_t>(continueIndex), finallyBlock);
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeFlagSlot), finallyBlock);
            size_t jumpToFinally = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

            // Patch original jump to trampoline
            ctx.program.patchJump(continueJump, static_cast<uint64_t>(trampolineOffset));
            newContinueJumps.push_back(jumpToFinally);
        }

        // Finally block starts here
        size_t finallyOffset = ctx.program.getCurrentOffset();
        ctx.exceptionManager.setFinallyOffset(finallyOffset);

        // Emit FINALLY instruction
        ctx.emitter.emitWithLocation(bytecode::OpCode::FINALLY, finallyBlock);

        // IMPORTANT: Initialize hasReturnFlagSlot to 0 (normal exit) as default
        // This runs when exception handler jumps to finallyOffset (FINALLY instruction)
        // Trampolines will jump to AFTER this initialization (afterInit), skipping it
        ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT, static_cast<uint64_t>(normalExitIndex), finallyBlock);
        ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeFlagSlot), finallyBlock);

        // afterInit: Trampolines jump here, skipping the default initialization above
        size_t afterInit = ctx.program.getCurrentOffset();

        // Patch trampoline jumps to afterInit (skip default initialization)
        for (size_t jump : newExitJumps) {
            ctx.emitter.patchJump(jump);
        }
        for (size_t jump : newReturnJumps) {
            ctx.emitter.patchJump(jump);
        }
        for (size_t jump : newBreakJumps) {
            ctx.emitter.patchJump(jump);
        }
        for (size_t jump : newContinueJumps) {
            ctx.emitter.patchJump(jump);
        }

        // Update exception table entries with finally offset
        std::string currentFunctionName = ctx.functionFrameManager.getCurrentFunctionName();
        if (currentFunctionName.empty()) {
            // Global scope
            if (noCatchBlocks) {
                // No catch blocks - create a finally-only entry
                bytecode::ExceptionTableEntry entry(
                    tryBeginOffset,      // startIP
                    tryEndOffset,        // endIP
                    SIZE_MAX,            // catchIP (no catch)
                    finallyOffset,       // finallyIP
                    "",                  // exceptionType (catch all)
                    nestingLevel,        // nestingLevel
                    "",                  // catchVarName (none)
                    SIZE_MAX             // catchVarSlot (none)
                );
                ctx.program.getGlobalExceptionTable().addEntry(entry);
            } else {
                // Update existing catch entries with finally offset
                ctx.program.getGlobalExceptionTable().updateFinallyIP(tryBeginOffset, tryEndOffset, nestingLevel, finallyOffset);
            }

            // IMPORTANT: Also create exception table entries for catch block bodies
            // so exceptions thrown from catch blocks also go through finally
            for (const auto& catchRange : catchRanges) {
                bytecode::ExceptionTableEntry catchEntry(
                    catchRange.startIP,      // startIP
                    catchRange.endIP,        // endIP
                    SIZE_MAX,                // catchIP (no catch for exceptions in catch blocks)
                    finallyOffset,           // finallyIP (go to finally)
                    "",                      // exceptionType (catch all)
                    nestingLevel,            // nestingLevel
                    "",                      // catchVarName (none)
                    SIZE_MAX                 // catchVarSlot (none)
                );
                ctx.program.getGlobalExceptionTable().addEntry(catchEntry);
            }
        } else {
            // Function scope
            auto* funcMetadata = const_cast<bytecode::BytecodeProgram::FunctionMetadata*>(
                ctx.program.getFunction(currentFunctionName)
            );
            if (funcMetadata) {
                if (noCatchBlocks) {
                    // No catch blocks - create a finally-only entry
                    bytecode::ExceptionTableEntry entry(
                        tryBeginOffset,      // startIP
                        tryEndOffset,        // endIP
                        SIZE_MAX,            // catchIP (no catch)
                        finallyOffset,       // finallyIP
                        "",                  // exceptionType (catch all)
                        nestingLevel,        // nestingLevel
                        "",                  // catchVarName (none)
                        SIZE_MAX             // catchVarSlot (none)
                    );
                    funcMetadata->exceptionTable.addEntry(entry);
                } else {
                    // Update existing catch entries with finally offset
                    funcMetadata->exceptionTable.updateFinallyIP(tryBeginOffset, tryEndOffset, nestingLevel, finallyOffset);
                }

                // IMPORTANT: Also create exception table entries for catch block bodies
                // so exceptions thrown from catch blocks also go through finally
                for (const auto& catchRange : catchRanges) {
                    bytecode::ExceptionTableEntry catchEntry(
                        catchRange.startIP,      // startIP
                        catchRange.endIP,        // endIP
                        SIZE_MAX,                // catchIP (no catch for exceptions in catch blocks)
                        finallyOffset,           // finallyIP (go to finally)
                        "",                      // exceptionType (catch all)
                        nestingLevel,            // nestingLevel
                        "",                      // catchVarName (none)
                        SIZE_MAX                 // catchVarSlot (none)
                    );
                    funcMetadata->exceptionTable.addEntry(catchEntry);
                }
            }
        }

        // Mark that we're now in the finally block
        ctx.exceptionManager.enterFinally();

        // Enter scope for finally block (variables shouldn't leak from try/catch)
        ctx.variableTracker.beginScope();

        // Compile finally body
        finallyBlock->accept(ctx.visitor);

        // Exit finally block scope
        ctx.variableTracker.endScope();
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        // Mark that we've exited the finally block
        ctx.exceptionManager.exitFinally();

        // After finally, check flag to determine what to do
        // Flag values: 0=normal exit, 1=return, 2=break, 3=continue

        // Check if flag == 1 (return)
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(relativeFlagSlot), finallyBlock);
        ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT, static_cast<uint64_t>(returnIndex), finallyBlock);
        ctx.emitter.emitWithLocation(bytecode::OpCode::EQ, finallyBlock);
        size_t jumpToReturnHandler = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_TRUE);

        // Check if flag == 2 (break)
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(relativeFlagSlot), finallyBlock);
        ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT, static_cast<uint64_t>(breakIndex), finallyBlock);
        ctx.emitter.emitWithLocation(bytecode::OpCode::EQ, finallyBlock);
        size_t jumpToBreakHandler = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_TRUE);

        // Check if flag == 3 (continue)
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(relativeFlagSlot), finallyBlock);
        ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT, static_cast<uint64_t>(continueIndex), finallyBlock);
        ctx.emitter.emitWithLocation(bytecode::OpCode::EQ, finallyBlock);
        size_t jumpToContinueHandler = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_TRUE);

        // Flag == 0 (normal exit) - fall through to continue execution
        size_t jumpOverHandlers = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Handle return path (flag == 1)
        ctx.emitter.patchJump(jumpToReturnHandler);
        // Note: returnValueSlot was already loaded at the beginning of this function
        if (returnValueSlot != SIZE_MAX) {
            // Convert absolute slot to relative slot for LOAD_LOCAL emission
            size_t relativeReturnSlot = returnValueSlot;
            if (ctx.functionFrameManager.isInFunction()) {
                size_t startSlot = ctx.functionFrameManager.currentFrame().localStartSlot;
                relativeReturnSlot = returnValueSlot - startSlot;
            }

            // Load the return value back from the special slot
            // Note: For async functions, the value is already wrapped in a Promise
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(relativeReturnSlot), finallyBlock);

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

                // Convert absolute slot to relative slot for STORE_LOCAL emission
                size_t relativeOuterReturnSlot = outerReturnValueSlot;
                if (ctx.functionFrameManager.isInFunction()) {
                    size_t startSlot = ctx.functionFrameManager.currentFrame().localStartSlot;
                    relativeOuterReturnSlot = outerReturnValueSlot - startSlot;
                }

                ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeOuterReturnSlot), finallyBlock);

                size_t jumpToOuterFinally = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
                ctx.exceptionManager.registerReturnJumpWithOuter(jumpToOuterFinally);
            } else {
                // No outer finally - emit RETURN_VALUE to actually return
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, finallyBlock);
            }
        }

        // Handle break path (flag == 2)
        ctx.emitter.patchJump(jumpToBreakHandler);
        // Emit a NEW break jump and register with loop manager
        // The loop will patch this to the loop end
        if (ctx.loopManager.isInLoop()) {
            size_t newBreakJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
            ctx.loopManager.registerBreak(newBreakJump);
        }

        // Handle continue path (flag == 3)
        ctx.emitter.patchJump(jumpToContinueHandler);
        // Emit a NEW continue jump and register with loop manager
        // The loop will patch this to the loop continue target
        if (ctx.loopManager.isInLoop()) {
            size_t newContinueJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
            ctx.loopManager.registerContinue(newContinueJump);
        }

        // Patch jump-over to land here (normal completion continues)
        ctx.emitter.patchJump(jumpOverHandlers);

        // Emit TRY_END to mark the end of the entire try-catch-finally construct
        // This helps the VM detect when it's safe to re-throw pending exceptions
        ctx.emitter.emitWithLocation(bytecode::OpCode::TRY_END, finallyBlock);

        // Normal completion path continues here to execute code after try-catch-finally
    }
}
