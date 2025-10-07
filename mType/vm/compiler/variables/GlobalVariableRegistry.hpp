#pragma once
#include "../../../value/ValueType.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vm::compiler::variables
{
    /**
     * Tracks global variables and their types
     * Manages compile-time validation of global variable access
     */
    class GlobalVariableRegistry
    {
    public:
        GlobalVariableRegistry() = default;
        ~GlobalVariableRegistry() = default;

        // Variable registration
        void registerGlobal(const std::string& name, value::ValueType type,
                          const std::string& className = "", int scopeDepth = 0);

        // Variable lookup
        bool exists(const std::string& name) const;
        value::ValueType getType(const std::string& name) const;
        std::string getClassName(const std::string& name) const;
        int getScopeDepth(const std::string& name) const;

        // Check if variable is in scope
        bool isInScope(const std::string& name, int currentScopeDepth) const;

        // Scope management
        void removeVariablesOutOfScope(int currentScopeDepth);

        // Clear all globals
        void clear();

    private:
        std::unordered_set<std::string> globalVariables;
        std::unordered_map<std::string, value::ValueType> globalVariableTypes;
        std::unordered_map<std::string, std::string> globalVariableClassNames;  // For OBJECT types
        std::unordered_map<std::string, int> globalVariableScopes;  // Track scope depth where global var was declared
    };
}
