#include "Environment.hpp"
#include "../errors/AmbiguousReferenceException.hpp"

namespace environment
{
    Environment::Environment(
        std::shared_ptr<ClassRegistry> classReg,
        std::shared_ptr<FunctionRegistry> functionReg,
        std::shared_ptr<VariableManager> varMgr,
        std::shared_ptr<ScopeManager> scopeMgr,
        std::shared_ptr<NativeRegistry> nativeReg
    ) : classRegistry(classReg),
        functionRegistry(functionReg),
        variableManager(varMgr),
        scopeManager(scopeMgr),
        nativeRegistry(nativeReg),
        importEvaluationActive(false),
        importManager(nullptr)
    {
    }

    void Environment::initialize()
    {
        if (classRegistry) classRegistry->initialize();
        if (functionRegistry) functionRegistry->initialize();
        if (variableManager) variableManager->initialize();
        if (scopeManager) scopeManager->initialize();
        if (nativeRegistry) nativeRegistry->initialize();
    }

    void Environment::cleanup()
    {
        if (classRegistry) classRegistry->cleanup();
        if (functionRegistry) functionRegistry->cleanup();
        if (variableManager) variableManager->cleanup();
        if (scopeManager) scopeManager->cleanup();
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


    std::shared_ptr<NativeRegistry> Environment::getNativeRegistry() const
    {
        return nativeRegistry;
    }

    void Environment::registerClass(const std::string& name, std::shared_ptr<ClassDefinition> classDefinition)
    {
        if (classRegistry)
        {
            classRegistry->registerClass(name, classDefinition);
        }
    }

    void Environment::registerFunction(const std::string& name, std::shared_ptr<FunctionDefinition> functionDefinition)
    {
        if (functionRegistry)
        {
            functionRegistry->registerFunction(name, functionDefinition);
        }
    }

    void Environment::declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable)
    {
        if (scopeManager)
        {
            scopeManager->declareVariable(varName, variable);
        }
        
        // Register in VariableManager for global scope
        if (variableManager && scopeManager && scopeManager->getCurrentScope() == scopeManager->getGlobalScope())
        {
            variableManager->declareVariable(varName, variable);
        }
    }

    std::shared_ptr<ClassDefinition> Environment::findClass(const std::string& name) const
    {
        if (!classRegistry) return nullptr;
        return classRegistry->findClass(name);
    }

    std::shared_ptr<FunctionDefinition> Environment::findFunction(const std::string& name) const
    {
        if (!functionRegistry) return nullptr;
        return functionRegistry->findFunction(name);
    }

    std::shared_ptr<VariableDefinition> Environment::findVariable(const std::string& name) const
    {
        // First check scope manager (for local variables)
        if (scopeManager)
        {
            if (auto var = scopeManager->findVariable(name))
            {
                return var;
            }
        }
        
        // Then check global scope variables
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
    }

    void Environment::exitScope()
    {
        if (scopeManager)
        {
            scopeManager->exitScope();
        }
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
    
    bool Environment::isEvaluatingImport() const
    {
        return importEvaluationActive;
    }
    
    void Environment::setImportEvaluation(bool active)
    {
        importEvaluationActive = active;
    }

    std::string Environment::getFunctionScopeName() const
    {
        return scopeManager ? scopeManager->getFunctionScopeName() : "";
    }

    void Environment::setImportManager(services::ImportManager* mgr)
    {
        importManager = mgr;
    }
    
    services::ImportManager* Environment::getImportManager() const
    {
        return importManager;
    }
    
    bool Environment::wouldCauseCircularImport(const std::string& filePath)
    {
        // Check if this file is already in the evaluation import stack
        std::stack<std::string> tempStack = evaluationImportStack;
        while (!tempStack.empty()) {
            if (tempStack.top() == filePath) {
                return true;
            }
            tempStack.pop();
        }
        return false;
    }
    
    void Environment::pushEvaluationImport(const std::string& filePath)
    {
        evaluationImportStack.push(filePath);
    }
    
    void Environment::popEvaluationImport()
    {
        if (!evaluationImportStack.empty()) {
            evaluationImportStack.pop();
        }
    }
    
    std::string Environment::getCircularImportChain(const std::string& filePath)
    {
        std::string chain = filePath;
        std::stack<std::string> tempStack = evaluationImportStack;
        
        while (!tempStack.empty()) {
            chain = tempStack.top() + " -> " + chain;
            if (tempStack.top() == filePath) {
                break; // Found the cycle
            }
            tempStack.pop();
        }
        
        return chain;
    }
}
