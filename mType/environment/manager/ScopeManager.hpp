#pragma once
#include "IManager.hpp"
#include "Scope.hpp"
#include <stack>
#include <memory>
#include <string>
#include <vector>

namespace environment::manager
{
    class ScopeManager : public IManager
    {
    private:
        std::shared_ptr<Scope> globalScope;
        std::shared_ptr<Scope> currentScope;
        std::stack<std::shared_ptr<Scope>> scopeStack;

    public:
        ScopeManager();
        ~ScopeManager() override = default;

        void enterScope(const std::string& scopeName = "") override;
        void exitScope() override;
        std::string getCurrentScopeName() const override;
        std::vector<std::string> getScopeHierarchy() const override;
        size_t getScopeDepth() const override;
        
        std::string getComponentName() const override;
        void initialize() override;
        void cleanup() override;
        
        void enterScope(const std::string& scopeName, ScopeType scopeType);
        std::shared_ptr<Scope> getCurrentScope() const;
        std::shared_ptr<Scope> getGlobalScope() const;
        
        void declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable);
        std::shared_ptr<VariableDefinition> findVariable(const std::string& varName) const;
        std::shared_ptr<VariableDefinition> findVariableInCurrentScope(const std::string& varName) const;
        bool hasVariable(const std::string& varName) const;
        bool hasVariableInCurrentScope(const std::string& varName) const;
        
        std::shared_ptr<Scope> findScope(const std::string& scopeName) const;
        std::vector<std::string> getCurrentNamespacePath() const;
        
        bool isInNamespace() const;
        bool isInClass() const;
        bool isInFunction() const;
        bool isInLoop() const;
        
        void pushScope(std::shared_ptr<Scope> scope);
        std::shared_ptr<Scope> popScope();
    };
}

