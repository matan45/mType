#include "LoopContextManager.hpp"
#include <cstdint>
#include "../../../errors/RuntimeException.hpp"

namespace vm::compiler::control
{
    void LoopContextManager::enterLoop(size_t loopStart, size_t continueTarget)
    {
        LoopContext ctx;
        ctx.loopStart = loopStart;
        ctx.continueTarget = (continueTarget == SIZE_MAX) ? loopStart : continueTarget;
        ctx.stackScopeDepthAtEntry = currentStackScopeDepth;
        loopStack.push_back(ctx);
    }

    void LoopContextManager::exitLoop()
    {
        if (loopStack.empty()) {
            throw errors::RuntimeException("Loop stack underflow");
        }
        loopStack.pop_back();
    }

    LoopContextManager::LoopContext& LoopContextManager::currentLoop()
    {
        if (loopStack.empty()) {
            throw errors::RuntimeException("Not in a loop context");
        }
        return loopStack.back();
    }

    bool LoopContextManager::isInLoop() const
    {
        return !loopStack.empty();
    }

    void LoopContextManager::registerBreak(size_t jumpOffset)
    {
        if (loopStack.empty()) {
            throw errors::RuntimeException("Break outside of loop");
        }
        loopStack.back().breakJumps.push_back(jumpOffset);
    }

    void LoopContextManager::registerContinue(size_t jumpOffset)
    {
        if (loopStack.empty()) {
            throw errors::RuntimeException("Continue outside of loop");
        }
        loopStack.back().continueJumps.push_back(jumpOffset);
    }

    size_t LoopContextManager::getLoopStart() const
    {
        if (loopStack.empty()) {
            throw errors::RuntimeException("Not in a loop context");
        }
        return loopStack.back().loopStart;
    }

    size_t LoopContextManager::getContinueTarget() const
    {
        if (loopStack.empty()) {
            throw errors::RuntimeException("Not in a loop context");
        }
        return loopStack.back().continueTarget;
    }

    const std::vector<size_t>& LoopContextManager::getBreakJumps() const
    {
        if (loopStack.empty()) {
            throw errors::RuntimeException("Not in a loop context");
        }
        return loopStack.back().breakJumps;
    }

    const std::vector<size_t>& LoopContextManager::getContinueJumps() const
    {
        if (loopStack.empty()) {
            throw errors::RuntimeException("Not in a loop context");
        }
        return loopStack.back().continueJumps;
    }

    uint32_t LoopContextManager::getStackScopeDepthDeltaForBreak() const
    {
        if (loopStack.empty()) return 0;
        const uint32_t entry = loopStack.back().stackScopeDepthAtEntry;
        return currentStackScopeDepth > entry ? (currentStackScopeDepth - entry) : 0;
    }
}
