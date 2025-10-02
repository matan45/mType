#include "ControlFlowManager.hpp"

namespace evaluator::managers
{
    ControlFlowManager::ControlFlowManager()
        : breakFlag(false), continueFlag(false), loopDepth(0), breakableDepth(0)
    {
    }

    bool ControlFlowManager::shouldBreakOrContinue() const
    {
        return breakFlag || continueFlag;
    }

    void ControlFlowManager::handleBreak()
    {
        breakFlag = true;
    }

    void ControlFlowManager::handleContinue()
    {
        continueFlag = true;
    }

    bool ControlFlowManager::isBreaking() const
    {
        return breakFlag;
    }

    bool ControlFlowManager::isContinuing() const
    {
        return continueFlag;
    }

    void ControlFlowManager::resetLoopFlags()
    {
        breakFlag = false;
        continueFlag = false;
    }

    void ControlFlowManager::enterLoop()
    {
        loopDepth++;
        breakableDepth++; // Loops are also breakable contexts
    }

    void ControlFlowManager::exitLoop()
    {
        if (loopDepth > 0)
        {
            loopDepth--;
        }
        if (breakableDepth > 0)
        {
            breakableDepth--;
        }
    }

    bool ControlFlowManager::isInLoop() const
    {
        return loopDepth > 0;
    }

    void ControlFlowManager::enterBreakableContext()
    {
        breakableDepth++;
    }

    void ControlFlowManager::exitBreakableContext()
    {
        if (breakableDepth > 0)
        {
            breakableDepth--;
        }
    }

    bool ControlFlowManager::isInBreakableContext() const
    {
        return breakableDepth > 0;
    }
}
