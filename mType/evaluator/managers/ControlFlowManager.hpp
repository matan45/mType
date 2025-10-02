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
        int loopDepth; // Track nested loop depth (for continue validation)
        int breakableDepth; // Track nested breakable contexts (loops, switches, etc.)
        
    public:
        explicit ControlFlowManager();
        ~ControlFlowManager() = default;
        
        // Break/Continue management
        bool shouldBreakOrContinue() const;
        void handleBreak();
        void handleContinue();
        bool isBreaking() const;
        bool isContinuing() const;
        void resetLoopFlags();

        // Loop depth management (for continue statement validation)
        void enterLoop();
        void exitLoop();
        bool isInLoop() const;

        // Breakable context management (for break statement validation)
        void enterBreakableContext(); // Enter any breakable context (loop, switch, etc.)
        void exitBreakableContext();  // Exit any breakable context
        bool isInBreakableContext() const; // Check if break statement is valid
        
        // Copy prevention
        ControlFlowManager(const ControlFlowManager&) = delete;
        ControlFlowManager& operator=(const ControlFlowManager&) = delete;
    };
}