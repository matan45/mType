#include "Environment.hpp"
#include "../circularDependency/TrueCyclicException.hpp"
#include "../circularDependency/DepthLimitException.hpp"

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
        interfaceRegistry(std::make_shared<runtimeTypes::klass::InterfaceRegistry>()),
        importEvaluationActive(false),
        importManager(nullptr)
    {
        // Initialize enhanced import dependency detection
        circularDependency::CircularDependencyConfig config;
        config.maxImportDepth = 100; // Allow deeper import chains than generic substitution
        config.enableEarlyDetection = true;
        config.enablePerformanceMetrics = true;

        importDependencyDetector = std::make_shared<circularDependency::CircularDependencyDetector>(config);
    }

    void Environment::initialize()
    {
        if (scopeManager) scopeManager->initialize();
        if (nativeRegistry) nativeRegistry->initialize();
    }

    void Environment::cleanup()
    {
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

    std::shared_ptr<runtimeTypes::klass::InterfaceRegistry> Environment::getInterfaceRegistry() const
    {
        return interfaceRegistry;
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
        // First check scope manager (for local variables with proper lexical scoping)
        if (scopeManager)
        {
            if (auto var = scopeManager->findVariable(name))
            {
                return var;
            }
        }

        // Only check global variables if we're in global scope
        // This prevents dynamic scoping behavior where variables from active function calls
        // become accessible to other functions
        if (variableManager && scopeManager && scopeManager->getCurrentScope() == scopeManager->getGlobalScope())
        {
            return variableManager->findVariable(name);
        }

        return nullptr;
    }

    void Environment::registerInterface(const std::string& name,
                                        std::shared_ptr<runtimeTypes::klass::InterfaceDefinition> interfaceDefinition)
    {
        if (interfaceRegistry)
        {
            interfaceRegistry->registerInterface(name, interfaceDefinition);
        }
    }

    std::shared_ptr<runtimeTypes::klass::InterfaceDefinition> Environment::findInterface(const std::string& name) const
    {
        if (!interfaceRegistry) return nullptr;
        return interfaceRegistry->findInterface(name);
    }

    void Environment::clearInterfaces()
    {
        if (interfaceRegistry)
        {
            interfaceRegistry->clearValidationCache();
            interfaceRegistry->clear();
        }
    }

    bool Environment::removeInterface(const std::string& name)
    {
        if (!interfaceRegistry) return false;
        return interfaceRegistry->removeInterface(name);
    }

    size_t Environment::cleanupUnusedInterfaces()
    {
        if (!interfaceRegistry) return 0;
        return interfaceRegistry->cleanupUnusedInterfaces();
    }

    std::vector<std::string> Environment::findUnusedInterfaces() const
    {
        if (!interfaceRegistry) return {};
        return interfaceRegistry->findUnusedInterfaces();
    }

    void Environment::enterScope(const std::string& scopeName, ScopeType scopeType)
    {
        if (scopeManager)
        {
            scopeManager->enterScope(scopeName, scopeType);
        }

        // NOTE: We don't notify VariableManager for scope changes as it only handles
        // global variables. All lexical scoping is handled by ScopeManager.
    }

    void Environment::exitScope()
    {
        if (scopeManager)
        {
            scopeManager->exitScope();
        }

        // NOTE: We don't notify VariableManager for scope changes as it only handles
        // global variables. All lexical scoping is handled by ScopeManager.
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
        // Check using enhanced circular dependency detection
        try
        {
            // Temporarily enter the dependency to see if it would cause a cycle
            importDependencyDetector->enterDependency(
                circularDependency::DependencyType::IMPORT_CHAIN,
                filePath,
                "import validation"
            );
            // If no exception, exit immediately and return false
            importDependencyDetector->exitDependency(
                circularDependency::DependencyType::IMPORT_CHAIN,
                filePath
            );
            return false;
        }
        catch (const circularDependency::TrueCyclicException&)
        {
            return true; // True circular dependency
        }
        catch (const circularDependency::DepthLimitException&)
        {
            return true; // Depth limit - treat as circular for safety
        }
    }

    bool Environment::enterImportDependency(const std::string& filePath, const std::string& location)
    {
        return importDependencyDetector->enterDependency(
            circularDependency::DependencyType::IMPORT_CHAIN,
            filePath,
            location
        );
    }

    void Environment::exitImportDependency(const std::string& filePath)
    {
        importDependencyDetector->exitDependency(
            circularDependency::DependencyType::IMPORT_CHAIN,
            filePath
        );
    }

    std::vector<std::string> Environment::getImportDependencyChain() const
    {
        return importDependencyDetector->getDependencyChain(circularDependency::DependencyType::IMPORT_CHAIN);
    }

    void Environment::setImportDependencyConfig(const circularDependency::CircularDependencyConfig& config)
    {
        importDependencyDetector->setConfig(config);
    }

    circularDependency::CircularDependencyConfig Environment::getImportDependencyConfig() const
    {
        return importDependencyDetector->getConfig();
    }

    void Environment::pushEvaluationImport(const std::string& filePath)
    {
        evaluationImportStack.push(filePath);
    }

    void Environment::popEvaluationImport()
    {
        if (!evaluationImportStack.empty())
        {
            evaluationImportStack.pop();
        }
    }

    std::string Environment::getCircularImportChain(const std::string& filePath)
    {
        std::string chain = filePath;
        std::stack<std::string> tempStack = evaluationImportStack;

        while (!tempStack.empty())
        {
            chain = tempStack.top() + " -> " + chain;
            if (tempStack.top() == filePath)
            {
                break; // Found the cycle
            }
            tempStack.pop();
        }

        return chain;
    }
}
