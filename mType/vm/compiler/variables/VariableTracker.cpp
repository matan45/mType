#include "VariableTracker.hpp"
#include  <iostream>
namespace vm::compiler::variables
{
    VariableTracker::VariableTracker()
        : nextLocalSlot(0), currentScopeDepth(0)
    {
    }

    size_t VariableTracker::resolveLocal(const std::string& name, size_t startSlot) const
    {
        // Search locals from the current function frame
        for (size_t i = locals.size(); i > 0; --i) {
            const LocalVariable& local = locals[i - 1];
            // Only consider locals from the current function (slot >= startSlot)
            if (local.slot >= startSlot && local.name == name) {
                return local.slot - startSlot;  // Return relative slot within this frame
            }
        }
        return SIZE_MAX;  // Not found
    }

    void VariableTracker::declareLocal(const std::string& name, value::ValueType type, const std::string& className)
    {
        LocalVariable local;
        local.name = name;
        local.slot = nextLocalSlot++;
        local.scopeDepth = currentScopeDepth;
        local.type = type;
        local.className = className;
        locals.push_back(local);
    }

    void VariableTracker::beginScope()
    {
        currentScopeDepth++;
    }

    void VariableTracker::endScope()
    {
        currentScopeDepth--;

        // Pop locals that went out of scope
        // Only decrement nextLocalSlot if the removed variable wasn't captured
        while (!locals.empty() && locals.back().scopeDepth > currentScopeDepth) {
            bool wasCaptured = locals.back().isCaptured;

            locals.pop_back();
            if (!wasCaptured) {
                nextLocalSlot--;
            }
        }
    }

    int VariableTracker::getCurrentScopeDepth() const
    {
        return currentScopeDepth;
    }

    size_t VariableTracker::getNextLocalSlot() const
    {
        return nextLocalSlot;
    }

    void VariableTracker::incrementLocalSlot()
    {
        nextLocalSlot++;
    }

    void VariableTracker::decrementLocalSlot()
    {
        nextLocalSlot--;
    }

    void VariableTracker::setNextLocalSlot(size_t slot)
    {
        nextLocalSlot = slot;
    }

    void VariableTracker::resetLocalSlot()
    {
        nextLocalSlot = 0;
    }

    void VariableTracker::setLocalSlot(size_t slot)
    {
        nextLocalSlot = slot;
    }

    const std::vector<VariableTracker::LocalVariable>& VariableTracker::getLocals() const
    {
        return locals;
    }

    void VariableTracker::clearLocals()
    {
        locals.clear();
    }

    void VariableTracker::removeLocalsFromSlot(size_t startSlot)
    {
        while (!locals.empty() && locals.back().slot >= startSlot) {
            locals.pop_back();
        }
    }

    bool VariableTracker::existsInCurrentScope(const std::string& name) const
    {
        for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
            if (it->scopeDepth < currentScopeDepth) {
                break; // Different scope, no conflict
            }
            if (it->name == name) {
                return true;
            }
        }
        return false;
    }

    bool VariableTracker::existsInFunction(const std::string& name) const
    {
        // Check if variable exists anywhere in the current function
        // This is used for parameter shadowing detection
        for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
            if (it->name == name) {
                return true;
            }
        }
        return false;
    }

    std::string VariableTracker::getLocalClassNameByName(const std::string& name) const
    {
        // Search for the most recent declaration of the variable
        for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
            if (it->name == name) {
                return it->className;
            }
        }
        return "";  // Not found
    }

    void VariableTracker::markVariableAsCaptured(size_t slot)
    {
        // Find the variable with this slot and mark it as captured
        for (auto& local : locals) {
            if (local.slot == slot) {
                local.isCaptured = true;
                return;
            }
        }
    }
}
