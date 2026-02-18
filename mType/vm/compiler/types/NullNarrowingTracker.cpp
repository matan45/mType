#include "NullNarrowingTracker.hpp"

namespace vm::compiler::types
{
    void NullNarrowingTracker::enterScope()
    {
        scopeStack.emplace_back();
    }

    void NullNarrowingTracker::exitScope()
    {
        if (!scopeStack.empty())
        {
            scopeStack.pop_back();
        }
    }

    void NullNarrowingTracker::narrowToNonNull(const std::string& varName)
    {
        if (!scopeStack.empty())
        {
            scopeStack.back().narrowedToNonNull.insert(varName);
        }
    }

    bool NullNarrowingTracker::isNarrowedNonNull(const std::string& varName) const
    {
        // Search from innermost scope outward
        for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it)
        {
            if (it->narrowedToNonNull.count(varName) > 0)
            {
                return true;
            }
        }
        return false;
    }
}
