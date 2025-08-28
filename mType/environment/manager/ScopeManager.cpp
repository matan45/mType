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
        
        auto newScope = std::make_shared<Scope>(scopeName, scopeType, currentScope);
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

    std::vector<std::string> ScopeManager::getCurrentNamespacePath() const
    {
        return currentScope ? currentScope->getNamespacePath() : std::vector<std::string>{};
    }

    bool ScopeManager::isInNamespace() const
    {
        if (!currentScope) return false;
        
        auto scope = currentScope;
        while (scope)
        {
            if (scope->getType() == ScopeType::NAMESPACE) return true;
            scope = scope->getParent();
        }
        return false;
    }

    bool ScopeManager::isInClass() const
    {
        if (!currentScope) return false;
        
        auto scope = currentScope;
        while (scope)
        {
            if (scope->getType() == ScopeType::CLASS) return true;
            scope = scope->getParent();
        }
        return false;
    }

    bool ScopeManager::isInFunction() const
    {
        if (!currentScope) return false;
        
        auto scope = currentScope;
        while (scope)
        {
            if (scope->getType() == ScopeType::FUNCTION) return true;
            scope = scope->getParent();
        }
        return false;
    }

    bool ScopeManager::isInLoop() const
    {
        if (!currentScope) return false;
        
        auto scope = currentScope;
        while (scope)
        {
            if (scope->getType() == ScopeType::LOOP) return true;
            scope = scope->getParent();
        }
        return false;
    }

    void ScopeManager::pushScope(std::shared_ptr<Scope> scope)
    {
        if (scope)
        {
            scopeStack.push(currentScope);
            currentScope = scope;
        }
    }

    std::shared_ptr<Scope> ScopeManager::popScope()
    {
        auto previousScope = currentScope;
        
        if (!scopeStack.empty())
        {
            currentScope = scopeStack.top();
            scopeStack.pop();
        }
        else
        {
            currentScope = globalScope;
        }
        
        return previousScope;
    }
}
