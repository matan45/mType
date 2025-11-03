#include "ExceptionContextManager.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace vm::compiler::control
{
    void ExceptionContextManager::enterTry(size_t tryBeginOffset, bool hasFinally)
    {
        ExceptionContext ctx;
        ctx.tryBeginOffset = tryBeginOffset;
        ctx.tryEndOffset = SIZE_MAX; // Will be set later
        ctx.finallyOffset = SIZE_MAX; // Will be set when finally is compiled
        ctx.hasFinally = hasFinally;
        ctx.inFinally = false; // Not in finally yet
        ctx.returnValueSlot = SIZE_MAX; // Will be set if a return happens in try with finally
        ctx.hasReturnFlagSlot = SIZE_MAX; // Will be set if finally needs to distinguish return vs exit paths
        contextStack.push_back(ctx);
    }

    void ExceptionContextManager::exitTry()
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Exception context stack underflow");
        }
        contextStack.pop_back();
    }

    ExceptionContextManager::ExceptionContext& ExceptionContextManager::currentContext()
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Not in an exception handling context");
        }
        return contextStack.back();
    }

    bool ExceptionContextManager::isInTry() const
    {
        return !contextStack.empty();
    }

    void ExceptionContextManager::registerCatchHandler(const std::string& exceptionType,
                                                       const std::string& variableName,
                                                       size_t handlerOffset)
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Catch handler outside of try context");
        }

        CatchHandler handler;
        handler.exceptionType = exceptionType;
        handler.variableName = variableName;
        handler.handlerOffset = handlerOffset;

        contextStack.back().catchHandlers.push_back(handler);
    }

    void ExceptionContextManager::setFinallyOffset(size_t finallyOffset)
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Finally block outside of try context");
        }
        contextStack.back().finallyOffset = finallyOffset;
    }

    void ExceptionContextManager::registerExitJump(size_t jumpOffset)
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Exit jump outside of try context");
        }
        contextStack.back().exitJumps.push_back(jumpOffset);
    }

    void ExceptionContextManager::registerReturnJump(size_t jumpOffset)
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Return jump outside of try context");
        }
        contextStack.back().returnJumps.push_back(jumpOffset);
    }

    void ExceptionContextManager::registerBreakJump(size_t jumpOffset)
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Break jump outside of try context");
        }
        contextStack.back().breakJumps.push_back(jumpOffset);
    }

    void ExceptionContextManager::registerContinueJump(size_t jumpOffset)
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Continue jump outside of try context");
        }
        contextStack.back().continueJumps.push_back(jumpOffset);
    }

    void ExceptionContextManager::setTryEndOffset(size_t offset)
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Try end outside of try context");
        }
        contextStack.back().tryEndOffset = offset;
    }

    size_t ExceptionContextManager::getTryBeginOffset() const
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Not in an exception handling context");
        }
        return contextStack.back().tryBeginOffset;
    }

    size_t ExceptionContextManager::getTryEndOffset() const
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Not in an exception handling context");
        }
        return contextStack.back().tryEndOffset;
    }

    size_t ExceptionContextManager::getFinallyOffset() const
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Not in an exception handling context");
        }
        return contextStack.back().finallyOffset;
    }

    const std::vector<ExceptionContextManager::CatchHandler>& ExceptionContextManager::getCatchHandlers() const
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Not in an exception handling context");
        }
        return contextStack.back().catchHandlers;
    }

    const std::vector<size_t>& ExceptionContextManager::getExitJumps() const
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Not in an exception handling context");
        }
        return contextStack.back().exitJumps;
    }

    const std::vector<size_t>& ExceptionContextManager::getReturnJumps() const
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Not in an exception handling context");
        }
        return contextStack.back().returnJumps;
    }

    const std::vector<size_t>& ExceptionContextManager::getBreakJumps() const
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Not in an exception handling context");
        }
        return contextStack.back().breakJumps;
    }

    const std::vector<size_t>& ExceptionContextManager::getContinueJumps() const
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Not in an exception handling context");
        }
        return contextStack.back().continueJumps;
    }

    void ExceptionContextManager::setReturnValueSlot(size_t slot)
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Return value slot set outside of try context");
        }
        contextStack.back().returnValueSlot = slot;
    }

    size_t ExceptionContextManager::getReturnValueSlot() const
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Not in an exception handling context");
        }
        return contextStack.back().returnValueSlot;
    }

    bool ExceptionContextManager::hasPendingFinally() const
    {
        if (contextStack.empty()) {
            return false;
        }
        // Check if current try block has a finally block AND we're not already in it
        return contextStack.back().hasFinally && !contextStack.back().inFinally;
    }

    void ExceptionContextManager::enterFinally()
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("enterFinally called outside of try context");
        }
        contextStack.back().inFinally = true;
    }

    void ExceptionContextManager::exitFinally()
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("exitFinally called outside of try context");
        }
        contextStack.back().inFinally = false;
    }

    bool ExceptionContextManager::isInFinally() const
    {
        if (contextStack.empty()) {
            return false;
        }
        return contextStack.back().inFinally;
    }

    bool ExceptionContextManager::hasOuterFinally() const
    {
        // Check if there's a parent context (at least 2 contexts on stack)
        // and the parent has a finally block
        if (contextStack.size() < 2) {
            return false;
        }
        // Check the second-to-last context (parent of current)
        const ExceptionContext& outerContext = contextStack[contextStack.size() - 2];
        return outerContext.hasFinally;
    }

    void ExceptionContextManager::registerReturnJumpWithOuter(size_t jumpOffset)
    {
        if (contextStack.size() < 2) {
            throw errors::RuntimeException("No outer context to register return jump with");
        }
        // Register with the second-to-last context (parent of current)
        contextStack[contextStack.size() - 2].returnJumps.push_back(jumpOffset);
    }

    void ExceptionContextManager::setReturnValueSlotForOuter(size_t slot)
    {
        if (contextStack.size() < 2) {
            throw errors::RuntimeException("No outer context to set return value slot for");
        }
        // Set for the second-to-last context (parent of current)
        contextStack[contextStack.size() - 2].returnValueSlot = slot;
    }

    size_t ExceptionContextManager::getReturnValueSlotForOuter() const
    {
        if (contextStack.size() < 2) {
            throw errors::RuntimeException("No outer context to get return value slot from");
        }
        // Get from the second-to-last context (parent of current)
        return contextStack[contextStack.size() - 2].returnValueSlot;
    }

    void ExceptionContextManager::setHasReturnFlagSlot(size_t slot)
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Has return flag slot set outside of try context");
        }
        contextStack.back().hasReturnFlagSlot = slot;
    }

    size_t ExceptionContextManager::getHasReturnFlagSlot() const
    {
        if (contextStack.empty()) {
            throw errors::RuntimeException("Not in an exception handling context");
        }
        return contextStack.back().hasReturnFlagSlot;
    }
}
