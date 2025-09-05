#include "ControlFlowManager.hpp"

namespace evaluator::managers
{
    ControlFlowManager::ControlFlowManager()
        : breakFlag(false), continueFlag(false)
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
}