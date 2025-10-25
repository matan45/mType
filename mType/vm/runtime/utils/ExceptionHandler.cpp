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
            else if (functionName.find("<lambda>") != std::string::npos)
            {
                // Lambda function - search forward to find the actual end
                // Need to handle multiple RETURNs (e.g., one in try, one in catch, one in finally)
                // Strategy: Find all RETURNs and take the last one at the lambda's scope level
                size_t lastReturn = SIZE_MAX;
                int callDepth = 0; // Track nested function calls

                for (size_t searchIP = currentIP + 1; searchIP < program->getInstructionCount(); ++searchIP)
                {
                    const auto& instr = program->getInstruction(searchIP);

                    // Track function call/return nesting
                    if (instr.opcode == bytecode::OpCode::CALL ||
                        instr.opcode == bytecode::OpCode::CALL_METHOD ||
                        instr.opcode == bytecode::OpCode::CALL_STATIC ||
                        instr.opcode == bytecode::OpCode::LAMBDA_INVOKE) {
                        callDepth++;
                    }

                    if (instr.opcode == bytecode::OpCode::RETURN ||
                        instr.opcode == bytecode::OpCode::RETURN_VALUE) {
                        if (callDepth == 0) {
                            // This is a return at our lambda's scope level
                            lastReturn = searchIP;
                            // Don't break - keep searching for more RETURNs
                        } else {
                            // This return belongs to a nested function call
                            callDepth--;
                        }
                    }

                    // Stop if we hit another lambda or function definition
                    if (instr.opcode == bytecode::OpCode::LAMBDA && searchIP != currentIP) {
                        break;
                    }
                }

                if (lastReturn != SIZE_MAX) {
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
        // But only accept CATCH/FINALLY that belongs to the same try block
        int tryDepth = 0; // Track nesting depth
        while (searchIP < searchLimit)
        {
            const auto& searchInstr = program->getInstruction(searchIP);

            // Track try-catch nesting
            if (searchInstr.opcode == bytecode::OpCode::TRY_BEGIN) {
                tryDepth++;
            }

            if (searchInstr.opcode == bytecode::OpCode::CATCH)
            {
                // Only accept CATCH if it's at the same nesting level (tryDepth == 0)
                // This ensures we don't catch with a CATCH from a different try-catch block
                if (tryDepth > 0) {
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
                        return result;
                    }
                }
            }
            else if (searchInstr.opcode == bytecode::OpCode::FINALLY)
            {
                // Only accept FINALLY if it's at the same nesting level
                if (tryDepth > 0) {
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
                return result;
            }
            else if (searchInstr.opcode == bytecode::OpCode::TRY_END) {
                // TRY_END marks the end of a try block's body
                // If we're at depth > 0, this ends the nested try we entered
                if (tryDepth > 0) {
                    tryDepth--;
                }
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
