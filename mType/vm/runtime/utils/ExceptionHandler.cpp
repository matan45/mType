#include "ExceptionHandler.hpp"

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
            if (funcMetadata)
            {
                searchLimit = funcMetadata->startOffset + funcMetadata->instructionCount;
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

        // User exception thrown - need to find matching catch handler
        size_t searchIP = currentIP + 1;
        size_t searchLimit = determineSearchLimit(currentIP);

        // First search: Look for CATCH or FINALLY in current scope
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
                        // Check if the CATCH is beyond the current function's boundary
                        // If so, unwind call frames until we reach the function containing the CATCH
                        unwindCallFrames(searchIP);

                        // Push exception object onto stack for STORE_LOCAL
                        stackManager->push(e.getExceptionValue());

                        // Jump to the CATCH instruction
                        result.handled = true;
                        result.newInstructionPointer = searchIP;
                        return result;
                    }
                }
            }
            else if (searchInstr.opcode == bytecode::OpCode::FINALLY)
            {
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
                return result;
            }

            searchIP++;
        }

        // No handler found in current scope - check if we're in a function call
        if (!callStack.empty())
        {
            // Pop the call frame
            CallFrame frame = callStack.back();
            callStack.pop_back();

            // Clean up the stack - pop all locals from the function
            cleanupStack(frame.frameBase);

            // Restore instruction pointer to the caller (right after the CALL)
            searchIP = frame.returnAddress;

            // Search again from caller's context
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
                    return result;
                }
                else if (searchInstr.opcode == bytecode::OpCode::RETURN ||
                         searchInstr.opcode == bytecode::OpCode::RETURN_VALUE)
                {
                    break;
                }

                searchIP++;
            }
        }

        // No matching catch found anywhere
        result.handled = false;
        return result;
    }
}
