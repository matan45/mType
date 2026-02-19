#pragma once
#include "../../../value/ValueType.hpp"
#include <string>
#include <vector>
#include <cstddef>

namespace vm::compiler::variables
{
    /**
     * Tracks local variables within function scopes
     * Manages variable slots and scope depth for proper variable resolution
     */
    class VariableTracker
    {
    public:
        struct LocalVariable {
            std::string name;
            size_t slot;
            int scopeDepth;
            value::ValueType type = value::ValueType::VOID;
            std::string className;  // For OBJECT types (interfaces/classes)
            bool isCaptured = false;  // True if captured by a lambda
            bool isNullable = false;  // True if type is nullable (T?)
        };

        VariableTracker();
        ~VariableTracker() = default;

        // Variable resolution
        size_t resolveLocal(const std::string& name, size_t startSlot) const;

        // Variable declaration
        void declareLocal(const std::string& name, value::ValueType type, const std::string& className = "", bool isNullable = false);

        // Scope management
        void beginScope();
        void endScope();
        int getCurrentScopeDepth() const;

        // Slot management
        size_t getNextLocalSlot() const;
        void incrementLocalSlot();
        void decrementLocalSlot();
        void setNextLocalSlot(size_t slot);
        void resetLocalSlot();  // Reset to 0 (for lambda compilation)
        void setLocalSlot(size_t slot);  // Alias for setNextLocalSlot

        // Local variable access
        const std::vector<LocalVariable>& getLocals() const;
        void clearLocals();
        void removeLocalsFromSlot(size_t startSlot);

        // Check if variable exists in current scope
        bool existsInCurrentScope(const std::string& name) const;

        // Check if variable exists in any scope within current function (for parameter shadowing detection)
        bool existsInFunction(const std::string& name) const;

        // Get class name for a local variable by name
        std::string getLocalClassNameByName(const std::string& name) const;

        // Get nullable status for a local variable by name
        bool getLocalNullableByName(const std::string& name) const;

        // Mark a variable as captured by a lambda
        void markVariableAsCaptured(size_t slot);

    private:
        std::vector<LocalVariable> locals;
        size_t nextLocalSlot;
        int currentScopeDepth;
    };
}
