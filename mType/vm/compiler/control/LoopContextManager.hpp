#pragma once
#include <vector>
#include <cstddef>

namespace vm::compiler::control
{
    /**
     * Manages loop context for break and continue statement handling
     * Tracks jump offsets that need to be patched when exiting loops
     */
    class LoopContextManager
    {
    public:
        struct LoopContext {
            std::vector<size_t> breakJumps;    // Jump instructions that need to be patched to loop end
            std::vector<size_t> continueJumps; // Jump instructions that need to be patched to continue target
            size_t loopStart;                   // Offset of loop start (condition check)
            size_t continueTarget;              // Offset where continue should jump (for loops: increment, others: loopStart)
        };

        LoopContextManager() = default;
        ~LoopContextManager() = default;

        // Loop management
        void enterLoop(size_t loopStart, size_t continueTarget = SIZE_MAX);
        void exitLoop();
        LoopContext& currentLoop();
        bool isInLoop() const;

        // Break/Continue registration
        void registerBreak(size_t jumpOffset);
        void registerContinue(size_t jumpOffset);

        // Get current loop information
        size_t getLoopStart() const;
        size_t getContinueTarget() const;
        const std::vector<size_t>& getBreakJumps() const;
        const std::vector<size_t>& getContinueJumps() const;

    private:
        std::vector<LoopContext> loopStack;
    };
}
