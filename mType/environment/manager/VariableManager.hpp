#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <stack>
#include "../../runtimeTypes/global/VariableDefinition.hpp"


namespace environment::manager
{
    using namespace runtimeTypes::global;

    // Represents a single scope level
    struct VariableScope
    {
        std::unordered_map<std::string, std::shared_ptr<VariableDefinition>> variables;
        std::string scopeName;

        VariableScope(const std::string& name = "") : scopeName(name) {}
    };

    class VariableManager
    {
    private:
        // Global scope (bottom of stack)
        std::unordered_map<std::string, std::shared_ptr<VariableDefinition>> globalVariables;

        // Scope stack for local variables (functions, blocks, etc.)
        std::stack<VariableScope> scopeStack;

        // Track current scope hierarchy
        std::vector<std::string> scopeHierarchy;

    public:
        explicit VariableManager() = default;
        ~VariableManager() = default;

        std::string getComponentName() const;
        void cleanup();

        // Scope management
        void enterScope(const std::string& scopeName = "");
        void exitScope();
        std::string getCurrentScopeName() const;
        std::vector<std::string> getScopeHierarchy() const;
        size_t getScopeDepth() const;

        // Variable management
        void declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable);
        std::shared_ptr<VariableDefinition> findVariable(const std::string& varName) const;
        bool hasVariable(const std::string& varName) const;

        // Check if variable exists in current scope only (for redefinition checks)
        bool hasVariableInCurrentScope(const std::string& varName) const;

        std::vector<std::string> getAllVariableNames() const;
        size_t getVariableCount() const;

        void removeVariable(const std::string& varName);
    };
}
