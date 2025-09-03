#pragma once

namespace evaluator::managers
{
    /**
     * @brief Manages control flow state for loops and switch statements
     * Following Single Responsibility Principle - separated from StatementEvaluator
     */
    class ControlFlowManager
    {
    private:
        bool breakFlag;
        bool continueFlag;
        
    public:
        ControlFlowManager();
        ~ControlFlowManager() = default;
        
        // Break/Continue management
        bool shouldBreakOrContinue() const;
        void handleBreak();
        void handleContinue();
        bool isBreaking() const;
        bool isContinuing() const;
        void resetLoopFlags();
        
        // Copy prevention
        ControlFlowManager(const ControlFlowManager&) = delete;
        ControlFlowManager& operator=(const ControlFlowManager&) = delete;
    };
}