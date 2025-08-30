#pragma once
#include "registry/ClassRegistry.hpp"
#include "registry/FunctionRegistry.hpp"
#include "registry/NativeRegistry.hpp"
#include "manager/VariableManager.hpp"
#include "manager/ScopeManager.hpp"
#include "manager/NamespaceManager.hpp"

#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include <memory>
#include <string>
#include <vector>
#include <stack>

// Forward declaration for clean architecture
namespace services { class ImportManager; }

namespace environment
{
    using namespace runtimeTypes::klass;
    using namespace runtimeTypes::global;
    using namespace registry;
    using namespace manager;

    class Environment
    {
    private:
        std::shared_ptr<ClassRegistry> classRegistry;
        std::shared_ptr<FunctionRegistry> functionRegistry;
        std::shared_ptr<VariableManager> variableManager;
        std::shared_ptr<ScopeManager> scopeManager;
        std::shared_ptr<NamespaceManager> namespaceManager;
        std::shared_ptr<NativeRegistry> nativeRegistry;
        
        // Import evaluation tracking
        bool importEvaluationActive;
        
        // Import manager for clean architecture  
        services::ImportManager* importManager;
        
        // Evaluation-level import stack for circular dependency detection
        std::stack<std::string> evaluationImportStack;

    public:
        explicit Environment(
            std::shared_ptr<ClassRegistry> classReg,
            std::shared_ptr<FunctionRegistry> functionReg,
            std::shared_ptr<VariableManager> varMgr,
            std::shared_ptr<ScopeManager> scopeMgr,
            std::shared_ptr<NamespaceManager> nsMgr,
            std::shared_ptr<NativeRegistry> nativeReg
        );
        
        ~Environment() = default;

        void initialize();
        void cleanup();

        std::shared_ptr<ClassRegistry> getClassRegistry() const;
        std::shared_ptr<FunctionRegistry> getFunctionRegistry() const;
        std::shared_ptr<VariableManager> getVariableManager() const;
        std::shared_ptr<ScopeManager> getScopeManager() const;
        std::shared_ptr<NamespaceManager> getNamespaceManager() const;
        std::shared_ptr<NativeRegistry> getNativeRegistry() const;
        
        // Import management
        void setImportManager(services::ImportManager* importManager);
        services::ImportManager* getImportManager() const;
        
        // Evaluation-level circular dependency detection
        bool wouldCauseCircularImport(const std::string& filePath);
        void pushEvaluationImport(const std::string& filePath);
        void popEvaluationImport();
        std::string getCircularImportChain(const std::string& filePath);

        void registerClass(const std::string& name, std::shared_ptr<ClassDefinition> classDefinition);
        void registerFunction(const std::string& name, std::shared_ptr<FunctionDefinition> functionDefinition);
        void declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable);

        std::shared_ptr<ClassDefinition> findClass(const std::string& name) const;
        std::shared_ptr<FunctionDefinition> findFunction(const std::string& name) const;
        std::shared_ptr<VariableDefinition> findVariable(const std::string& name) const;

        void enterScope(const std::string& scopeName = "", ScopeType scopeType = ScopeType::BLOCK);
        void exitScope();
        void enterNamespace(const std::vector<std::string>& namespacePath);
        void exitNamespace();

        std::vector<std::string> getCurrentNamespacePath() const;
        std::string getCurrentScopeName() const;
        std::vector<std::string> getScopeHierarchy() const;
        size_t getScopeDepth() const;

        void addUsingDirective(const std::vector<std::string>& namespacePath);
        const std::vector<std::vector<std::string>>& getUsingDirectives() const;

        bool isInNamespace() const;
        bool isInClass() const;
        bool isInFunction() const;
        bool isInLoop() const;
        
        // Import evaluation context
        bool isEvaluatingImport() const;
        void setImportEvaluation(bool active);
        
        std::string getFunctionScopeName() const;

        std::vector<std::string> resolveQualifiedName(const std::string& name) const;
    };
}

