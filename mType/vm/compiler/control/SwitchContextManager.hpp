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
        };

        SwitchContextManager() = default;
        ~SwitchContextManager() = default;

        // Switch management
        void enterSwitch();
        void exitSwitch();
        SwitchContext& currentSwitch();
        bool isInSwitch() const;

        // Break registration
        void registerBreak(size_t jumpOffset);

        // Get current switch information
        const std::vector<size_t>& getBreakJumps() const;

    private:
        std::vector<SwitchContext> switchStack;
    };
}
