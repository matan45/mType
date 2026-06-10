#pragma once
#include <vector>
#include <cstddef>

namespace vm::compiler::control
{
    /**
     * Manages switch context for break statement handling
     * Tracks jump offsets that need to be patched when exiting switch statements
     */
    class SwitchContextManager
    {
    public:
        struct SwitchContext {
            std::vector<size_t> breakJumps;    // Jump instructions that need to be patched to switch end
            // Bytecode offset at which this switch was entered. Strictly
            // monotonic across constructs, so compileBreak can compare it
            // against the innermost loop's start offset to bind a break to
            // whichever construct was entered later (i.e. nested deeper).
            size_t entryOffset = 0;
        };

        SwitchContextManager() = default;
        ~SwitchContextManager() = default;

        // Switch management
        void enterSwitch(size_t entryOffset);
        void exitSwitch();
        SwitchContext& currentSwitch();
        bool isInSwitch() const;

        // Break registration
        void registerBreak(size_t jumpOffset);

        // Get current switch information
        const std::vector<size_t>& getBreakJumps() const;
        // Entry offset of the innermost switch (used for break-target nesting order).
        size_t getCurrentSwitchEntryOffset() const;

    private:
        std::vector<SwitchContext> switchStack;
    };
}
