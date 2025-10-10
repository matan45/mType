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
        ctx.returnValueSlot = SIZE_MAX; // Will be set if a return happens in try with finally
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
        // Check if current try block has a finally block
        return contextStack.back().hasFinally;
    }
}
