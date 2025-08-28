#include "Environment.hpp"

namespace environment
{
    Environment::Environment(
        std::shared_ptr<ClassRegistry> classReg,
        std::shared_ptr<FunctionRegistry> functionReg,
        std::shared_ptr<VariableManager> varMgr,
        std::shared_ptr<ScopeManager> scopeMgr,
        std::shared_ptr<NamespaceManager> nsMgr,
        std::shared_ptr<NativeRegistry> nativeReg
    ) : classRegistry(classReg),
        functionRegistry(functionReg),
        variableManager(varMgr),
        scopeManager(scopeMgr),
        namespaceManager(nsMgr),
        nativeRegistry(nativeReg)
    {
    }

    void Environment::initialize()
    {
        if (classRegistry) classRegistry->initialize();
        if (functionRegistry) functionRegistry->initialize();
        if (variableManager) variableManager->initialize();
        if (scopeManager) scopeManager->initialize();
        if (namespaceManager) namespaceManager->initialize();
        if (nativeRegistry) nativeRegistry->initialize();
    }

    void Environment::cleanup()
    {
        if (classRegistry) classRegistry->cleanup();
        if (functionRegistry) functionRegistry->cleanup();
        if (variableManager) variableManager->cleanup();
        if (scopeManager) scopeManager->cleanup();
        if (namespaceManager) namespaceManager->cleanup();
        if (nativeRegistry) nativeRegistry->cleanup();
    }

    std::shared_ptr<ClassRegistry> Environment::getClassRegistry() const
    {
        return classRegistry;
    }

    std::shared_ptr<FunctionRegistry> Environment::getFunctionRegistry() const
    {
        return functionRegistry;
    }

    std::shared_ptr<VariableManager> Environment::getVariableManager() const
    {
        return variableManager;
    }

    std::shared_ptr<ScopeManager> Environment::getScopeManager() const
    {
        return scopeManager;
    }

    std::shared_ptr<NamespaceManager> Environment::getNamespaceManager() const
    {
        return namespaceManager;
    }

    std::shared_ptr<NativeRegistry> Environment::getNativeRegistry() const
    {
        return nativeRegistry;
    }

    void Environment::registerClass(const std::string& name, std::shared_ptr<ClassDefinition> classDefinition)
    {
        if (classRegistry)
        {
            auto namespacePath = getCurrentNamespacePath();
            if (namespacePath.empty())
            {
                classRegistry->registerClass(name, classDefinition);
            }
            else
            {
                classRegistry->registerClassInNamespace(namespacePath, name, classDefinition);
            }
        }
    }

    void Environment::registerFunction(const std::string& name, std::shared_ptr<FunctionDefinition> functionDefinition)
    {
        if (functionRegistry)
        {
            auto namespacePath = getCurrentNamespacePath();
            if (namespacePath.empty())
            {
                functionRegistry->registerFunction(name, functionDefinition);
            }
            else
            {
                functionRegistry->registerFunctionInNamespace(namespacePath, name, functionDefinition);
            }
        }
    }

    void Environment::declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable)
    {
        if (scopeManager)
        {
            scopeManager->declareVariable(varName, variable);
        }
        
        if (variableManager)
        {
            variableManager->declareVariable(varName, variable);
        }
    }

    std::shared_ptr<ClassDefinition> Environment::findClass(const std::string& name) const
    {
        if (!classRegistry) return nullptr;
        
        auto namespacePath = getCurrentNamespacePath();
        if (!namespacePath.empty())
        {
            if (auto cls = classRegistry->findClassInNamespace(namespacePath, name))
            {
                return cls;
            }
        }
        
        return classRegistry->findClass(name);
    }

    std::shared_ptr<FunctionDefinition> Environment::findFunction(const std::string& name) const
    {
        if (!functionRegistry) return nullptr;
        
        auto namespacePath = getCurrentNamespacePath();
        if (!namespacePath.empty())
        {
            if (auto func = functionRegistry->findFunctionInNamespace(namespacePath, name))
            {
                return func;
            }
        }
        
        return functionRegistry->findFunction(name);
    }

    std::shared_ptr<VariableDefinition> Environment::findVariable(const std::string& name) const
    {
        if (scopeManager)
        {
            if (auto var = scopeManager->findVariable(name))
            {
                return var;
            }
        }
        
        if (variableManager)
        {
            return variableManager->findVariable(name);
        }
        
        return nullptr;
    }

    void Environment::enterScope(const std::string& scopeName, ScopeType scopeType)
    {
        if (scopeManager)
        {
            scopeManager->enterScope(scopeName, scopeType);
        }
        
        if (scopeType == ScopeType::NAMESPACE && namespaceManager)
        {
            auto currentPath = getCurrentNamespacePath();
            currentPath.push_back(scopeName);
            namespaceManager->enterNamespace(currentPath);
        }
    }

    void Environment::exitScope()
    {
        bool wasNamespace = false;
        if (scopeManager && scopeManager->getCurrentScope())
        {
            wasNamespace = scopeManager->getCurrentScope()->getType() == ScopeType::NAMESPACE;
            scopeManager->exitScope();
        }
        
        if (wasNamespace && namespaceManager)
        {
            namespaceManager->exitNamespace();
        }
    }

    void Environment::enterNamespace(const std::vector<std::string>& namespacePath)
    {
        if (namespaceManager)
        {
            namespaceManager->enterNamespace(namespacePath);
        }
        
        if (scopeManager && !namespacePath.empty())
        {
            scopeManager->enterScope(namespacePath.back(), ScopeType::NAMESPACE);
        }
    }

    void Environment::exitNamespace()
    {
        if (namespaceManager)
        {
            namespaceManager->exitNamespace();
        }
        
        if (scopeManager)
        {
            scopeManager->exitScope();
        }
    }

    std::vector<std::string> Environment::getCurrentNamespacePath() const
    {
        if (namespaceManager)
        {
            return namespaceManager->getCurrentNamespacePath();
        }
        return {};
    }

    std::string Environment::getCurrentScopeName() const
    {
        if (scopeManager)
        {
            return scopeManager->getCurrentScopeName();
        }
        return "unknown";
    }

    std::vector<std::string> Environment::getScopeHierarchy() const
    {
        if (scopeManager)
        {
            return scopeManager->getScopeHierarchy();
        }
        return {};
    }

    size_t Environment::getScopeDepth() const
    {
        if (scopeManager)
        {
            return scopeManager->getScopeDepth();
        }
        return 0;
    }

    void Environment::addUsingDirective(const std::vector<std::string>& namespacePath)
    {
        if (namespaceManager)
        {
            namespaceManager->addUsingDirective(namespacePath);
        }
    }

    const std::vector<std::vector<std::string>>& Environment::getUsingDirectives() const
    {
        static const std::vector<std::vector<std::string>> empty;
        if (namespaceManager)
        {
            return namespaceManager->getUsingDirectives();
        }
        return empty;
    }

    bool Environment::isInNamespace() const
    {
        return scopeManager ? scopeManager->isInNamespace() : false;
    }

    bool Environment::isInClass() const
    {
        return scopeManager ? scopeManager->isInClass() : false;
    }

    bool Environment::isInFunction() const
    {
        return scopeManager ? scopeManager->isInFunction() : false;
    }

    bool Environment::isInLoop() const
    {
        return scopeManager ? scopeManager->isInLoop() : false;
    }

    std::vector<std::string> Environment::resolveQualifiedName(const std::string& name) const
    {
        if (namespaceManager)
        {
            return namespaceManager->resolveQualifiedName(name);
        }
        return {name};
    }
}
