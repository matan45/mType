#include "ExceptionHandler.hpp"
#include "../../bytecode/OpCode.hpp"
#include  <iostream>
namespace vm::runtime::utils
{
    ExceptionHandler::ExceptionHandler(const bytecode::BytecodeProgram* prog,
                                       std::shared_ptr<StackManager> stackMgr,
                                       std::vector<CallFrame>& callStack)
        : program(prog)
          , stackManager(stackMgr)
          , callStack(callStack)
    {
    }

    size_t ExceptionHandler::determineSearchLimit(size_t currentIP) const
    {
        size_t searchLimit = program->getInstructionCount();

        if (!callStack.empty())
        {
            // We're in a function - find its end boundary
            const std::string& functionName = callStack.back().functionName;
            auto* funcMetadata = program->getFunction(functionName);

            // If function metadata not found and this is a lambda, try using the lambda's actual function name
            if (!funcMetadata && callStack.back().originatingLambda)
            {
                const std::string& lambdaFuncName = callStack.back().originatingLambda->functionName;
                funcMetadata = program->getFunction(lambdaFuncName);
            }

            if (funcMetadata)
            {
                searchLimit = funcMetadata->startOffset + funcMetadata->instructionCount;
            }
            else if (functionName.find("<lambda>") != std::string::npos)
            {
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

            // Check if the target IP is within the current function's range
            auto* funcMetadata = program->getFunction(frame.functionName);
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
            else if (frame.functionName.find("<lambda>") != std::string::npos)
            {
                // Lambda function - don't unwind if CATCH/FINALLY is inside the lambda
                // This prevents incorrectly unwinding the lambda's call frame when
                // the exception handler is within the lambda itself
                break;
            }

            // Target is outside this function - unwind the call frame
            callStack.pop_back();

            // Clean up the stack
            cleanupStack(frame.frameBase);
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

    ExceptionHandler::HandlingResult ExceptionHandler::handleUserException(
        const errors::UserException& e,
        size_t currentIP,
        size_t currentFinallyOffset)
    {
        HandlingResult result;
        result.handled = false;
        result.newInstructionPointer = currentIP;
        result.jumpedToFinally = false;

        // User exception thrown - need to find matching catch handler
        size_t searchIP = currentIP + 1;
        size_t searchLimit = determineSearchLimit(currentIP);

        // First search: Look for CATCH or FINALLY in current scope
        // But only accept CATCH/FINALLY that belongs to the same try block
        int tryDepth = 0; // Track nesting depth
        std::vector<int> tryEndCounts; // Track how many TRY_ENDs we've seen for each nested try
        while (searchIP < searchLimit && searchIP < program->getInstructionCount())
        {
            const auto& searchInstr = program->getInstruction(searchIP);

            // Track try-catch nesting
            if (searchInstr.opcode == bytecode::OpCode::TRY_BEGIN)
            {
                tryDepth++;
                tryEndCounts.push_back(0); // Start counting TRY_ENDs for this nested try
            }

            if (searchInstr.opcode == bytecode::OpCode::CATCH)
            {
                // Only accept CATCH if it's at the same nesting level (tryDepth == 0)
                // This ensures we don't catch with a CATCH from a different try-catch block
                if (tryDepth > 0)
                {
                    searchIP++;
                    continue;
                }

                // Found a catch block - check if it matches the exception type
                if (!searchInstr.operands.empty())
                {
                    std::string catchType = program->getConstantPool().getString(searchInstr.operands[0]);

                    if (e.matchesCatchType(catchType))
                    {
                        // Check if the CATCH is beyond the current function's boundary
                        // If so, unwind call frames until we reach the function containing the CATCH
                        unwindCallFrames(searchIP);

                        // Push exception object onto stack for STORE_LOCAL
                        stackManager->push(e.getExceptionValue());

                        // Jump to the CATCH instruction
                        result.handled = true;
                        result.newInstructionPointer = searchIP;
                        result.jumpedToFinally = false;  // Jumped to CATCH, exception is caught
                        return result;
                    }
                }
            }
            else if (searchInstr.opcode == bytecode::OpCode::FINALLY)
            {
                // Only accept FINALLY if it's at the same nesting level
                if (tryDepth > 0)
                {
                    searchIP++;
                    continue;
                }

                // Check if this is the currently executing finally block
                if (isInFinallyBlock(searchIP, currentFinallyOffset))
                {
                    // This is the finally we're already in - skip it
                    searchIP++;
                    continue;
                }

                // Hit a different finally block - execute it then re-throw
                result.handled = true;
                result.newInstructionPointer = searchIP;
                result.jumpedToFinally = true;  // Jumped to FINALLY, need to re-throw after
                return result;
            }
            else if (searchInstr.opcode == bytecode::OpCode::TRY_END)
            {
                // TRY_END appears twice in a try-catch-finally construct:
                // 1. After try body (before CATCH blocks)
                // 2. After all CATCH/FINALLY blocks (end of entire construct)
                // We should only decrement tryDepth after seeing BOTH TRY_ENDs
                if (tryDepth > 0 && !tryEndCounts.empty())
                {
                    tryEndCounts.back()++;

                    // If we've seen 2 TRY_ENDs, we've exited the entire nested try-catch-finally
                    if (tryEndCounts.back() >= 2)
                    {
                        tryDepth--;
                        tryEndCounts.pop_back();
                    }
                }
            }

            searchIP++;
        }

        // No handler found in current scope - unwind call stack to search callers
        while (!callStack.empty())
        {
            // Pop the call frame
            CallFrame frame = callStack.back();
            callStack.pop_back();

            // Clean up the stack - pop all locals from the function
            cleanupStack(frame.frameBase);

            // Restore instruction pointer to the caller (right after the CALL)
            searchIP = frame.returnAddress;

            // Safety check: ensure searchIP is within bounds
            if (searchIP >= program->getInstructionCount())
            {
                continue;  // Try next call frame
            }

            // Search for handler in this caller's context
            while (searchIP < program->getInstructionCount())
            {
                const auto& searchInstr = program->getInstruction(searchIP);

                if (searchInstr.opcode == bytecode::OpCode::CATCH)
                {
                    if (!searchInstr.operands.empty())
                    {
                        std::string catchType = program->getConstantPool().getString(searchInstr.operands[0]);

                        if (e.matchesCatchType(catchType))
                        {
                            // Check if the CATCH is beyond the current function's boundary
                            // If so, unwind more call frames
                            unwindCallFrames(searchIP);

                            stackManager->push(e.getExceptionValue());
                            result.handled = true;
                            result.newInstructionPointer = searchIP;
                            result.jumpedToFinally = false;  // Jumped to CATCH, exception is caught
                            return result;
                        }
                    }
                }
                else if (searchInstr.opcode == bytecode::OpCode::FINALLY)
                {
                    if (isInFinallyBlock(searchIP, currentFinallyOffset))
                    {
                        searchIP++;
                        continue;
                    }

                    result.handled = true;
                    result.newInstructionPointer = searchIP;
                    result.jumpedToFinally = true;  // Jumped to FINALLY, need to re-throw after
                    return result;
                }
                else if (searchInstr.opcode == bytecode::OpCode::RETURN ||
                    searchInstr.opcode == bytecode::OpCode::RETURN_VALUE)
                {
                    // Hit end of this function - no handler found in this frame
                    // Break inner loop to continue unwinding to next call frame
                    break;
                }

                searchIP++;
            }

            // No handler found in this frame, continue to next frame (outer while loop)
        }

        // Call stack is now empty - no handler found anywhere
        result.handled = false;
        return result;
    }
}
