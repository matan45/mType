#include "ScopeManager.hpp"

namespace environment::manager
{
    ScopeManager::ScopeManager()
    {
        globalScope = std::make_shared<Scope>("global", ScopeType::GLOBAL);
        currentScope = globalScope;
    }

    void ScopeManager::enterScope(const std::string& scopeName)
    {
        enterScope(scopeName, ScopeType::BLOCK);
    }

    void ScopeManager::exitScope()
    {
        if (currentScope && currentScope != globalScope)
        {
            if (!scopeStack.empty())
            {
                currentScope = scopeStack.top();
                scopeStack.pop();
            }
            else
            {
                currentScope = globalScope;
            }
        }
    }

    std::string ScopeManager::getCurrentScopeName() const
    {
        return currentScope ? currentScope->getName() : "";
    }

    std::vector<std::string> ScopeManager::getScopeHierarchy() const
    {
        return currentScope ? currentScope->getScopeHierarchy() : std::vector<std::string>{};
    }

    size_t ScopeManager::getScopeDepth() const
    {
        return currentScope ? currentScope->getDepth() : 0;
    }

    std::string ScopeManager::getComponentName() const
    {
        return "ScopeManager";
    }

    void ScopeManager::initialize()
    {
        globalScope = std::make_shared<Scope>("global", ScopeType::GLOBAL);
        currentScope = globalScope;
        
        while (!scopeStack.empty())
        {
            scopeStack.pop();
        }
    }

    void ScopeManager::cleanup()
    {
        while (!scopeStack.empty())
        {
            scopeStack.pop();
        }
        
        currentScope = globalScope;
    }

    void ScopeManager::enterScope(const std::string& scopeName, ScopeType scopeType)
    {
        scopeStack.push(currentScope);

        // For lexical scoping: function scopes should have global scope as parent,
        // not the dynamically active scope
        std::shared_ptr<Scope> parentScope = currentScope;
        if (scopeType == ScopeType::FUNCTION) {
            parentScope = globalScope; // Functions use lexical scoping - parent is global, not caller
        }

        auto newScope = std::make_shared<Scope>(scopeName, scopeType, parentScope);
        currentScope->addChild(newScope);
        currentScope = newScope;
    }

    std::shared_ptr<Scope> ScopeManager::getCurrentScope() const
    {
        return currentScope;
    }

    std::shared_ptr<Scope> ScopeManager::getGlobalScope() const
    {
        return globalScope;
    }

    void ScopeManager::declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable)
    {
        if (currentScope)
        {
            currentScope->declareVariable(varName, variable);
        }
    }

    std::shared_ptr<VariableDefinition> ScopeManager::findVariable(const std::string& varName) const
    {
        return currentScope ? currentScope->findVariable(varName) : nullptr;
    }

    std::shared_ptr<VariableDefinition> ScopeManager::findVariableInCurrentScope(const std::string& varName) const
    {
        return currentScope ? currentScope->findVariableInCurrentScope(varName) : nullptr;
    }

    bool ScopeManager::hasVariable(const std::string& varName) const
    {
        return currentScope ? currentScope->hasVariable(varName) : false;
    }

    bool ScopeManager::hasVariableInCurrentScope(const std::string& varName) const
    {
        return currentScope ? currentScope->hasVariableInCurrentScope(varName) : false;
    }

    std::shared_ptr<Scope> ScopeManager::findScope(const std::string& scopeName) const
    {
        return globalScope ? globalScope->findScope(scopeName) : nullptr;
    }

    bool ScopeManager::isInScopeType(ScopeType targetType) const
    {
        if (!currentScope) return false;

        auto scope = currentScope;
        while (scope)
        {
            if (scope->getType() == targetType) return true;
            scope = scope->getParent();
        }
        return false;
    }

    bool ScopeManager::isInClass() const
    {
        return isInScopeType(ScopeType::CLASS);
    }

    bool ScopeManager::isInFunction() const
    {
        return isInScopeType(ScopeType::FUNCTION);
    }

    bool ScopeManager::isInLoop() const
    {
        return isInScopeType(ScopeType::LOOP);
    }

    std::string ScopeManager::getFunctionScopeName() const
    {
        if (!currentScope) return "";

        auto scope = currentScope;
        while (scope)
        {
            if (scope->getType() == ScopeType::FUNCTION) return scope->getName();
            scope = scope->getParent();
        }
        return "";
    }
}
