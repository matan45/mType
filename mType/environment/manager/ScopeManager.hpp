#pragma once
#include "Scope.hpp"
#include <cstddef>
#include <stack>
#include <memory>
#include <string>
#include <vector>

namespace environment::manager
{
    class ScopeManager
    {
    private:
        std::shared_ptr<Scope> globalScope;
        std::shared_ptr<Scope> currentScope;
        std::stack<std::shared_ptr<Scope>> scopeStack;

        // Helper method to check if currently in a specific scope type
        bool isInScopeType(ScopeType targetType) const;

    public:
       explicit  ScopeManager();
        ~ScopeManager() = default;

        void enterScope(const std::string& scopeName = "");
        void exitScope();
        std::string getCurrentScopeName() const;
        std::vector<std::string> getScopeHierarchy() const;
        size_t getScopeDepth() const;
        
        std::string getComponentName() const;
        void initialize();
        void cleanup();
        
        void enterScope(const std::string& scopeName, ScopeType scopeType);
        std::shared_ptr<Scope> getCurrentScope() const;
        std::shared_ptr<Scope> getGlobalScope() const;
        
        void declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable);
        std::shared_ptr<VariableDefinition> findVariable(const std::string& varName) const;
        std::shared_ptr<VariableDefinition> findVariableInCurrentScope(const std::string& varName) const;
        bool hasVariable(const std::string& varName) const;
        bool hasVariableInCurrentScope(const std::string& varName) const;
        
        std::shared_ptr<Scope> findScope(const std::string& scopeName) const;
        
        bool isInClass() const;
        bool isInFunction() const;
        bool isInLoop() const;
        std::string getFunctionScopeName() const;
    };
}

