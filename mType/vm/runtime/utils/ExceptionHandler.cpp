#include "ExceptionHandler.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../profiler/ProfilerHookHelper.hpp"

namespace vm::runtime::utils
{
    ExceptionHandler::ExceptionHandler(const bytecode::BytecodeProgram*& progRef,
                                       std::shared_ptr<StackManager> stackMgr,
                                       std::vector<CallFrame>& callStack,
                                       std::vector<const bytecode::BytecodeProgram*>* loadedProgs)
        : program(progRef)
          , stackManager(stackMgr)
          , callStack(callStack)
          , loadedPrograms(loadedProgs)
    {
    }

    void ExceptionHandler::restoreProgramForCurrentFrame()
    {
        if (!loadedPrograms || callStack.empty()) return;
        const auto& frame = callStack.back();
        if (frame.programIndex < loadedPrograms->size()) {
            program = (*loadedPrograms)[frame.programIndex];
        }
    }

    size_t ExceptionHandler::determineSearchLimit(size_t currentIP) const
    {
        size_t searchLimit = program->getInstructionCount();

        if (!callStack.empty())
        {
            // We're in a function - find its end boundary.
            // MYT-197: O(1) handle lookup replaces the std::string hashmap probe.
            bytecode::FunctionNameHandle functionNameHandle = callStack.back().functionName;
            auto* funcMetadata = program->getFunctionMeta(functionNameHandle);

            // If function metadata not found and this is a lambda, try using the lambda's actual function name.
            if (!funcMetadata && callStack.back().originatingLambda)
            {
                bytecode::FunctionNameHandle lambdaFuncHandle = callStack.back().originatingLambda->functionName;
                funcMetadata = program->getFunctionMeta(lambdaFuncHandle);
            }

            if (funcMetadata)
            {
                searchLimit = funcMetadata->startOffset + funcMetadata->instructionCount;
            }
            else if (callStack.back().originatingLambda)
            {
                // MYT-197: lambda frames carry originatingLambda — use that as
                // the lambda-detection signal instead of a <lambda> substring
                // search on the resolved frame name. The old substring path
                // fired for the "<lambda>" / "Class::<lambda>" display
                // fallbacks set at lambda push sites.
                // Lambda function without metadata - search forward to find the actual end
                // Need to handle multiple RETURNs (e.g., one in try, one in catch, one in finally)
                // Strategy: Find all RETURNs until we hit another LAMBDA (lambdas don't nest inline)
                // Note: CALL/LAMBDA_INVOKE don't introduce nested RETURNs in the same bytecode stream
                size_t lastReturn = SIZE_MAX;

                for (size_t searchIP = currentIP + 1; searchIP < program->getInstructionCount(); ++searchIP)
                {
                    const auto& instr = program->getInstruction(searchIP);

                    // Stop if we hit another lambda definition
                    if (instr.opcode == bytecode::OpCode::LAMBDA && searchIP != currentIP)
                    {
                        break;
                    }

                    // Record all RETURNs - they all belong to this lambda
                    if (instr.opcode == bytecode::OpCode::RETURN ||
                        instr.opcode == bytecode::OpCode::RETURN_VALUE)
                    {
                        lastReturn = searchIP;
                        // Don't break - keep searching for more RETURNs (e.g., in catch/finally blocks)
                    }
                }

                if (lastReturn != SIZE_MAX)
                {
                    searchLimit = lastReturn + 1; // Include the RETURN instruction
                }
            }
        }

        return searchLimit;
    }

    bool ExceptionHandler::searchForCatchBlock(const errors::UserException& e,
                                               size_t startIP,
                                               size_t searchLimit,
                                               size_t& catchIP) const
    {
        size_t searchIP = startIP;

        while (searchIP < searchLimit)
        {
            const auto& searchInstr = program->getInstruction(searchIP);

            if (searchInstr.opcode == bytecode::OpCode::CATCH)
            {
                // Found a catch block - check if it matches the exception type
                if (!searchInstr.operands.empty())
                {
                    std::string catchType = program->getConstantPool().getString(searchInstr.operands[0]);

                    if (e.matchesCatchType(catchType))
                    {
                        catchIP = searchIP;
                        return true;
                    }
                }
            }

            searchIP++;
        }

        return false;
    }

    void ExceptionHandler::unwindCallFrames(size_t targetIP)
    {
        while (!callStack.empty())
        {
            const CallFrame& frame = callStack.back();

            // Safety check: if targetIP is beyond program bounds, stop unwinding
            if (targetIP >= program->getInstructionCount())
            {
                break;
            }

            // Check if the target IP is within the current function's range.
            // MYT-197: O(1) handle-keyed metadata lookup.
            auto* funcMetadata = program->getFunctionMeta(frame.functionName);
            if (funcMetadata)
            {
                size_t funcStart = funcMetadata->startOffset;
                size_t funcEnd = funcStart + funcMetadata->instructionCount;

                // If target is within this function, stop unwinding
                if (targetIP >= funcStart && targetIP < funcEnd)
                {
                    break;
                }
            }
            else if (frame.originatingLambda)
            {
                // MYT-197: lambda frames carry originatingLambda — use that
                // signal instead of a <lambda> substring search on the
                // resolved frame name. Don't unwind if CATCH/FINALLY is
                // inside the lambda itself.
                break;
            }
            else
            {
                // MYT-208: __script_main__ has no FunctionMetadata (it's
                // synthesised by VirtualMachine, not registered via
                // BytecodeProgram::registerFunction). Without a range to
                // compare against, the previous logic fell through and
                // popped script_main during exception unwind — leaving the
                // catch handler running with no live frame and (now that
                // releaseStackObjects fires here) freeing top-level locals
                // that the rest of the script still uses. The script-main
                // frame's lifetime is the entire program; stop unwinding
                // here so the catch resumes inside script_main with all
                // top-level locals (and stack-promoted allocations like
                // `tests = new ExceptionFinallyTests()`) still alive.
                break;
            }

            if (vm::profiler::ProfilerHookHelper::isProfilingEnabled())
            {
                vm::profiler::ProfilerHookHelper::onFunctionExit(
                    program->getFrameName(frame.functionName));
            }

            // Target is outside this function - unwind the call frame.
            // MYT-208: release stack-promoted allocations BEFORE pop so the
            // pool gets the slot back on every unwind path.
            callStack.back().releaseStackObjects();
            callStack.pop_back();

            // Clean up the stack
            cleanupStack(frame.frameBase);

            // Restore program pointer when unwinding across library boundaries
            restoreProgramForCurrentFrame();
        }
    }

    bool ExceptionHandler::isInFinallyBlock(size_t catchIP, size_t currentFinallyOffset) const
    {
        return catchIP == currentFinallyOffset;
    }

    void ExceptionHandler::cleanupStack(size_t targetFrameBase)
    {
        // Safety check: don't try to pop more than what's on the stack
        size_t currentSize = stackManager->size();
        if (targetFrameBase >= currentSize)
        {
            return; // Nothing to clean up
        }

        while (stackManager->size() > targetFrameBase)
        {
            stackManager->getStack().pop_back();
        }
    }

    const bytecode::ExceptionTable* ExceptionHandler::getExceptionTable() const
    {
        // Determine which exception table to use based on current execution context
        if (callStack.empty())
        {
            // Global scope
            return &program->getGlobalExceptionTable();
        }

        // Get function name from current call frame.
        // MYT-197: try O(1) handle-keyed metadata first.
        bytecode::FunctionNameHandle functionNameHandle = callStack.back().functionName;
        const auto* funcMetadata = program->getFunctionMeta(functionNameHandle);
        if (funcMetadata && funcMetadata->exceptionTable.size() > 0)
        {
            return &funcMetadata->exceptionTable;
        }

        // Fallback: the call frame may store a bare name (e.g. "riskyDivide") while the
        // compiler registered the mangled name (e.g. "riskyDivide/int,int") with the
        // exception table. Search for a matching function by prefix. Cold path
        // — resolving the handle here is fine.
        if (!funcMetadata || funcMetadata->exceptionTable.size() == 0)
        {
            const std::string& functionName = program->getFrameName(functionNameHandle);
            for (const auto& [fname, fmeta] : program->getFunctions())
            {
                if (fname.rfind(functionName, 0) == 0 && fname.size() > functionName.size()
                    && (fname[functionName.size()] == '/' || fname[functionName.size()] == '$')
                    && fmeta.exceptionTable.size() > 0)
                {
                    return &fmeta.exceptionTable;
                }
            }
        }

        // Return empty exception table from exact match if it exists
        if (funcMetadata)
        {
            return &funcMetadata->exceptionTable;
        }

        // Lambda without metadata - check if originatingLambda has a function name
        if (callStack.back().originatingLambda)
        {
            bytecode::FunctionNameHandle lambdaFuncHandle =
                callStack.back().originatingLambda->functionName;
            const auto* lambdaMetadata = program->getFunctionMeta(lambdaFuncHandle);
            if (lambdaMetadata)
            {
                return &lambdaMetadata->exceptionTable;
            }
        }

        // Fallback to global table
        return &program->getGlobalExceptionTable();
    }

    ExceptionHandler::HandlingResult ExceptionHandler::handleUserException(
        const errors::UserException& e,
        size_t currentIP,
        size_t currentFinallyOffset)
    {
        HandlingResult result;
        result.handled = false;
        result.newInstructionPointer = currentIP;
        result.jumpedToFinally = false;

        // Try to find handler in current scope using exception table
        const bytecode::ExceptionTable* exceptionTable = getExceptionTable();
        const bytecode::ExceptionTableEntry* handler = exceptionTable->findHandler(currentIP, e.getExceptionTypeName(), e.getExceptionValue());

        if (handler)
        {
            // Found a handler in current scope
            // Check if this is the currently executing finally block (avoid infinite loop)
            if (handler->hasFinallyHandler() && isInFinallyBlock(handler->finallyIP, currentFinallyOffset))
            {
                // We're already in this finally block - don't jump to it again
                // Instead, fall through to unwind to caller
            }
            else if (handler->hasCatchHandler() && exceptionTable->isTypeCompatible(e.getExceptionTypeName(), handler->exceptionType, e.getExceptionValue()))
            {
                // Jump to CATCH handler (catch type matches the exception)
                // Unwind call frames if necessary (in case handler is in outer function)
                unwindCallFrames(handler->catchIP);

                // Push exception object onto stack for STORE_LOCAL
                stackManager->push(e.getExceptionValue());

                result.handled = true;
                result.newInstructionPointer = handler->catchIP;
                result.jumpedToFinally = false;  // Jumped to CATCH, exception is caught
                return result;
            }
            else if (handler->hasFinallyHandler())
            {
                // Jump to FINALLY handler (no catch, or catch didn't match)
                result.handled = true;
                result.newInstructionPointer = handler->finallyIP;
                result.jumpedToFinally = true;  // Jumped to FINALLY, need to re-throw after
                return result;
            }
        }

        // MYT-115: an async function's body is a catch-search boundary. If no
        // handler was found inside the async frame, leave it on the call stack
        // so VirtualMachineLoop's async-rejection path can convert the
        // uncaught exception into a rejection of the function's returned
        // Promise, rather than unwinding past it.
        auto isAsyncFrame = [&](const CallFrame& frame) -> bool {
            // MYT-197: O(1) handle-keyed metadata lookup.
            const auto* fm = program->getFunctionMeta(frame.functionName);
            return fm && fm->isAsync;
        };

        if (!callStack.empty() && isAsyncFrame(callStack.back()))
        {
            return result;  // handled=false, frame intact
        }

        // No handler found in current scope - unwind call stack to search callers
        // IMPORTANT: Save the returnAddress BEFORE popping the frame, as it tells us where
        // the current function was called from (the call site we need to check)
        size_t callSiteIP = SIZE_MAX;
        if (!callStack.empty())
        {
            CallFrame currentFrame = callStack.back();
            callSiteIP = currentFrame.returnAddress;  // Where this function was called from

            if (vm::profiler::ProfilerHookHelper::isProfilingEnabled())
            {
                vm::profiler::ProfilerHookHelper::onFunctionExit(
                    program->getFrameName(currentFrame.functionName));
            }

            // MYT-208: release stack-promoted allocations BEFORE pop.
            callStack.back().releaseStackObjects();
            callStack.pop_back();
            cleanupStack(currentFrame.frameBase);

            // Restore program pointer when unwinding across library boundaries
            restoreProgramForCurrentFrame();
        }

        // Check if the call site where the exception occurred is covered by the caller's exception table
        // IMPORTANT: Even if call stack is empty (unwound to global scope), we still need to check
        // the global exception table if we have a valid call site IP
        if (callSiteIP != SIZE_MAX)
        {
            // Get caller's exception table (will be global table if call stack is empty)
            const bytecode::ExceptionTable* callerTable = getExceptionTable();

            // Look for handler at call site in caller's exception table
            const bytecode::ExceptionTableEntry* callerHandler = callerTable->findHandler(callSiteIP, e.getExceptionTypeName(), e.getExceptionValue());

            if (callerHandler)
            {
                // Found handler in caller - check if it's the current finally
                if (callerHandler->hasFinallyHandler() && isInFinallyBlock(callerHandler->finallyIP, currentFinallyOffset))
                {
                    // Already in this finally - continue unwinding
                }
                else if (callerHandler->hasCatchHandler() && callerTable->isTypeCompatible(e.getExceptionTypeName(), callerHandler->exceptionType, e.getExceptionValue()))
                {
                    // Jump to caller's CATCH handler (catch type matches the exception)
                    unwindCallFrames(callerHandler->catchIP);

                    stackManager->push(e.getExceptionValue());
                    result.handled = true;
                    result.newInstructionPointer = callerHandler->catchIP;
                    result.jumpedToFinally = false;
                    return result;
                }
                else if (callerHandler->hasFinallyHandler())
                {
                    // Jump to caller's FINALLY handler (no catch, or catch didn't match)
                    result.handled = true;
                    result.newInstructionPointer = callerHandler->finallyIP;
                    result.jumpedToFinally = true;
                    return result;
                }
            }
        }

        // Continue searching in remaining caller frames
        while (!callStack.empty())
        {
            const CallFrame& frame = callStack.back();

            // MYT-115: stop at async frame boundary; see comment above.
            if (isAsyncFrame(frame))
            {
                return result;  // handled=false, async frame intact
            }

            size_t frameCallSite = frame.returnAddress;

            if (vm::profiler::ProfilerHookHelper::isProfilingEnabled())
            {
                vm::profiler::ProfilerHookHelper::onFunctionExit(
                    program->getFrameName(frame.functionName));
            }

            // Pop this frame FIRST, then check if the call site is covered by the caller's exception table
            // MYT-208: release stack-promoted allocations BEFORE pop. This is the
            // path taken when a throw originates inside a stack-promoted ctor's body.
            callStack.back().releaseStackObjects();
            callStack.pop_back();
            cleanupStack(frame.frameBase);

            // Get caller's exception table (returns global table when callStack is empty)
            const bytecode::ExceptionTable* frameTable = getExceptionTable();

            // Look for handler at the call site in the caller's exception table
            const bytecode::ExceptionTableEntry* frameHandler = frameTable->findHandler(frameCallSite, e.getExceptionTypeName(), e.getExceptionValue());

            if (frameHandler)
            {
                // Found handler - check if it's the current finally
                if (frameHandler->hasFinallyHandler() && isInFinallyBlock(frameHandler->finallyIP, currentFinallyOffset))
                {
                    // Already in this finally - continue to next frame
                    continue;
                }

                if (frameHandler->hasCatchHandler() && frameTable->isTypeCompatible(e.getExceptionTypeName(), frameHandler->exceptionType, e.getExceptionValue()))
                {
                    // Jump to CATCH handler (catch type matches the exception)
                    unwindCallFrames(frameHandler->catchIP);

                    stackManager->push(e.getExceptionValue());
                    result.handled = true;
                    result.newInstructionPointer = frameHandler->catchIP;
                    result.jumpedToFinally = false;
                    return result;
                }
                else if (frameHandler->hasFinallyHandler())
                {
                    // Jump to FINALLY handler (no catch, or catch didn't match)
                    result.handled = true;
                    result.newInstructionPointer = frameHandler->finallyIP;
                    result.jumpedToFinally = true;
                    return result;
                }
            }

            // No handler in this caller - already popped, continue to next frame
        }

        // Call stack is now empty - no handler found anywhere
        result.handled = false;
        return result;
    }
}
