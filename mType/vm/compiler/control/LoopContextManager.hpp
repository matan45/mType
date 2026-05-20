#pragma once
#include <vector>
#include <cstdint>
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
            // Active STACK_SCOPE_ENTER depth at the time this loop was entered.
            // When a break / continue inside the loop emits its JUMP, the
            // compiler synthesizes (currentStackScopeDepth - scopeDepthAtEntry)
            // STACK_SCOPE_LEAVE ops before the jump so the runtime pool
            // accounting stays consistent.
            uint32_t stackScopeDepthAtEntry = 0;
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

        // Stack-scope tracking for synthesized LEAVE emission on break/continue/return.
        // Each STACK_SCOPE_ENTER call notifies on enter; each LEAVE notifies on exit.
        void notifyStackScopeEnter() { ++currentStackScopeDepth; }
        void notifyStackScopeLeave() { if (currentStackScopeDepth > 0) --currentStackScopeDepth; }
        uint32_t getCurrentStackScopeDepth() const { return currentStackScopeDepth; }
        // Number of LEAVEs to synthesize before a break/continue jumps out of
        // the current loop (depth-at-now minus depth-at-loop-entry).
        uint32_t getStackScopeDepthDeltaForBreak() const;

    private:
        std::vector<LoopContext> loopStack;
        // Live count of STACK_SCOPE_ENTER ops emitted but not yet matched by
        // a STACK_SCOPE_LEAVE on the current compile path. Used to size
        // break/continue/return cleanup sequences. Compile-time only; doesn't
        // exist at runtime.
        uint32_t currentStackScopeDepth = 0;
    };
}
