#pragma once
#include <string>
#include <vector>
#include <unordered_set>

namespace vm::compiler::types
{
    /**
     * Tracks variables that have been narrowed to non-null via null checks.
     * Used for smart casts: after `if (x != null)`, x is treated as non-nullable
     * within the then-branch.
     */
    class NullNarrowingTracker
    {
    public:
        NullNarrowingTracker() = default;
        ~NullNarrowingTracker() = default;

        // Scope management for narrowing
        void enterScope();
        void exitScope();

        // Narrow a variable to non-null within the current scope
        void narrowToNonNull(const std::string& varName);

        // Check if a variable has been narrowed to non-null
        bool isNarrowedNonNull(const std::string& varName) const;

    private:
        struct NarrowingScope {
            std::unordered_set<std::string> narrowedToNonNull;
        };
        std::vector<NarrowingScope> scopeStack;
    };
}
