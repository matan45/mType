#pragma once
#include "registry/ClassRegistry.hpp"
#include <cstddef>
#include "registry/FunctionRegistry.hpp"
#include "registry/NativeRegistry.hpp"
#include "registry/ExportRegistry.hpp"
#include "registry/AnnotationRegistry.hpp"
#include "registry/TypeCatalog.hpp"
#include "manager/VariableManager.hpp"
#include "manager/ScopeManager.hpp"

#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/InterfaceRegistry.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../circularDependency/CircularDependencyDetector.hpp"
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

// Forward declaration for clean architecture
namespace services
{
    class ImportManager;
}

namespace environment
{
    using namespace runtimeTypes::klass;
    using namespace runtimeTypes::global;
    using namespace registry;
    using namespace manager;

    class Environment
    {
    private:
        // TypeCatalog inherits ClassRegistry, so one instance serves both
        // accessors. `classRegistry` is kept as a typed view for backward
        // compatibility with the many `getClassRegistry()->X()` call sites.
        std::shared_ptr<TypeCatalog> typeCatalog;
        std::shared_ptr<ClassRegistry> classRegistry;
        std::shared_ptr<FunctionRegistry> functionRegistry;
        std::shared_ptr<VariableManager> variableManager;
        std::shared_ptr<ScopeManager> scopeManager;
        std::shared_ptr<NativeRegistry> nativeRegistry;
        std::shared_ptr<runtimeTypes::klass::InterfaceRegistry> interfaceRegistry;
        std::shared_ptr<ExportRegistry> exportRegistry;
        std::shared_ptr<AnnotationRegistry> annotationRegistry;

        // Import evaluation tracking
        bool importEvaluationActive;

        // Import manager for clean architecture
        services::ImportManager* importManager;

        // Enhanced circular dependency detection for imports
        std::shared_ptr<circularDependency::CircularDependencyDetector> importDependencyDetector;

        // Loaded library tracking (for deduplication)
        std::unordered_set<std::string> loadedLibraryNames;

    public:
        explicit Environment(
            std::shared_ptr<ClassRegistry> classReg,
            std::shared_ptr<FunctionRegistry> functionReg,
            std::shared_ptr<VariableManager> varMgr,
            std::shared_ptr<ScopeManager> scopeMgr,
            std::shared_ptr<NativeRegistry> nativeReg
        );

        ~Environment() = default;

        void initialize();
        void cleanup();
        void resetForRebuild();

        std::shared_ptr<ClassRegistry> getClassRegistry() const;
        std::shared_ptr<TypeCatalog> getTypeCatalog() const;
        std::shared_ptr<FunctionRegistry> getFunctionRegistry() const;
        std::shared_ptr<VariableManager> getVariableManager() const;
        std::shared_ptr<ScopeManager> getScopeManager() const;
        std::shared_ptr<NativeRegistry> getNativeRegistry() const;
        std::shared_ptr<runtimeTypes::klass::InterfaceRegistry> getInterfaceRegistry() const;
        std::shared_ptr<ExportRegistry> getExportRegistry() const;
        std::shared_ptr<AnnotationRegistry> getAnnotationRegistry() const;

        // Import management
        void setImportManager(services::ImportManager* importManager);
        services::ImportManager* getImportManager() const;

        // Enhanced circular dependency detection for imports
        bool wouldCauseCircularImport(const std::string& filePath);
        bool enterImportDependency(const std::string& filePath, const std::string& location = "");
        void exitImportDependency(const std::string& filePath);
        std::vector<std::string> getImportDependencyChain() const;

        // Configuration for import dependency limits
        void setImportDependencyConfig(const circularDependency::CircularDependencyConfig& config);
        circularDependency::CircularDependencyConfig getImportDependencyConfig() const;

        // Library loading tracking
        bool isLibraryLoaded(const std::string& name) const { return loadedLibraryNames.count(name) > 0; }
        void markLibraryLoaded(const std::string& name) { loadedLibraryNames.insert(name); }
        [[nodiscard]] bool unmarkLibraryLoaded(const std::string& name) { return loadedLibraryNames.erase(name) > 0; }

        void registerClass(const std::string& name, std::shared_ptr<ClassDefinition> classDefinition);
        void registerFunction(const std::string& name, std::shared_ptr<FunctionDefinition> functionDefinition);
        void declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable);
        void registerInterface(const std::string& name, std::shared_ptr<runtimeTypes::klass::InterfaceDefinition> interfaceDefinition);

        std::shared_ptr<ClassDefinition> findClass(const std::string& name) const;
        std::shared_ptr<FunctionDefinition> findFunction(const std::string& name) const;
        std::shared_ptr<VariableDefinition> findVariable(const std::string& name) const;
        std::shared_ptr<runtimeTypes::klass::InterfaceDefinition> findInterface(const std::string& name) const;

        // Interface registry cleanup methods
        bool removeInterface(const std::string& name);
        size_t cleanupUnusedInterfaces();
        std::vector<std::string> findUnusedInterfaces() const;

        void enterScope(const std::string& scopeName = "", ScopeType scopeType = ScopeType::BLOCK);
        void exitScope();

        std::string getCurrentScopeName() const;
        std::vector<std::string> getScopeHierarchy() const;
        size_t getScopeDepth() const;

        bool isInClass() const;
        bool isInFunction() const;
        bool isInLoop() const;

        // Import evaluation context
        bool isEvaluatingImport() const;
        void setImportEvaluation(bool active);

        std::string getFunctionScopeName() const;
    };
}
