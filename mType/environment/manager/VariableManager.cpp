#include "VariableManager.hpp"
#include <algorithm>

namespace environment::manager
{
    std::string VariableManager::getComponentName() const
    {
        return "VariableManager";
    }

    void VariableManager::cleanup()
    {
        globalVariables.clear();
        while (!scopeStack.empty()) {
            scopeStack.pop();
        }
        scopeHierarchy.clear();
    }

    void VariableManager::enterScope(const std::string& scopeName)
    {
        scopeStack.emplace(scopeName);
        scopeHierarchy.push_back(scopeName);
    }

    void VariableManager::exitScope()
    {
        if (!scopeStack.empty()) {
            scopeStack.pop();
        }
        if (!scopeHierarchy.empty()) {
            scopeHierarchy.pop_back();
        }
    }

    std::string VariableManager::getCurrentScopeName() const
    {
        if (scopeStack.empty()) {
            return "global";
        }
        return scopeStack.top().scopeName;
    }

    std::vector<std::string> VariableManager::getScopeHierarchy() const
    {
        return scopeHierarchy;
    }

    size_t VariableManager::getScopeDepth() const
    {
        return scopeStack.size();
    }

    void VariableManager::declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable)
    {
        // If we're in a local scope, add to current scope
        if (!scopeStack.empty()) {
            scopeStack.top().variables[varName] = variable;
        } else {
            // Global scope
            globalVariables[varName] = variable;
        }
    }

    void VariableManager::declareGlobalVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable)
    {
        // Always declare in global scope, regardless of current scope
        globalVariables[varName] = variable;
    }

    std::shared_ptr<VariableDefinition> VariableManager::findVariable(const std::string& varName) const
    {
        // Create a copy of the scope stack for traversal
        std::stack<VariableScope> tempStack = scopeStack;

        // Search from innermost to outermost scope
        while (!tempStack.empty()) {
            const auto& currentScope = tempStack.top();
            auto it = currentScope.variables.find(varName);
            if (it != currentScope.variables.end()) {
                return it->second;
            }
            tempStack.pop();
        }

        // Finally check global scope
        auto it = globalVariables.find(varName);
        if (it != globalVariables.end()) {
            return it->second;
        }

        return nullptr;
    }

    bool VariableManager::hasVariable(const std::string& varName) const
    {
        return findVariable(varName) != nullptr;
    }

    bool VariableManager::hasVariableInCurrentScope(const std::string& varName) const
    {
        if (!scopeStack.empty()) {
            const auto& currentScope = scopeStack.top();
            return currentScope.variables.find(varName) != currentScope.variables.end();
        } else {
            // Global scope
            return globalVariables.find(varName) != globalVariables.end();
        }
    }

    std::vector<std::string> VariableManager::getAllVariableNames() const
    {
        std::vector<std::string> names;

        // Add global variables
        for (const auto& [name, _] : globalVariables) {
            names.push_back(name);
        }

        // Add variables from all local scopes
        std::stack<VariableScope> tempStack = scopeStack;
        while (!tempStack.empty()) {
            const auto& currentScope = tempStack.top();
            for (const auto& [name, _] : currentScope.variables) {
                names.push_back(name);
            }
            tempStack.pop();
        }

        std::sort(names.begin(), names.end());
        return names;
    }

    size_t VariableManager::getVariableCount() const
    {
        size_t count = globalVariables.size();

        // Count variables in all local scopes
        std::stack<VariableScope> tempStack = scopeStack;
        while (!tempStack.empty()) {
            count += tempStack.top().variables.size();
            tempStack.pop();
        }

        return count;
    }

    void VariableManager::removeVariable(const std::string& varName)
    {
        // Remove from current scope first
        if (!scopeStack.empty()) {
            scopeStack.top().variables.erase(varName);
        } else {
            globalVariables.erase(varName);
        }
    }
}
