#pragma once
#include "IManager.hpp"
#include "../util/NamespaceUtils.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

namespace environment::manager
{
    using namespace runtimeTypes::global;

    class VariableManager : public IManager
    {
    private:
        std::unordered_map<std::string, std::shared_ptr<VariableDefinition>> globalVariables;
        std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<VariableDefinition>>> namespacedVariables;
        std::vector<std::string> currentNamespacePath;

    public:
        VariableManager() = default;
        ~VariableManager() override = default;

        void enterScope(const std::string& scopeName = "") override;
        void exitScope() override;
        std::string getCurrentScopeName() const override;
        std::vector<std::string> getScopeHierarchy() const override;
        size_t getScopeDepth() const override;
        
        std::string getComponentName() const override;
        void initialize() override;
        void cleanup() override;
        
        void declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable);
        void declareVariableInNamespace(const std::vector<std::string>& namespacePath, const std::string& varName, std::shared_ptr<VariableDefinition> variable);
        std::shared_ptr<VariableDefinition> findVariable(const std::string& varName) const;
        std::shared_ptr<VariableDefinition> findVariableInNamespace(const std::vector<std::string>& namespacePath, const std::string& varName) const;
        bool hasVariable(const std::string& varName) const;
        bool hasVariableInNamespace(const std::vector<std::string>& namespacePath, const std::string& varName) const;
        
        void setCurrentNamespace(const std::vector<std::string>& namespacePath);
        const std::vector<std::string>& getCurrentNamespace() const;
        
        std::vector<std::string> getAllVariableNames() const;
        std::vector<std::string> getVariablesInNamespace(const std::vector<std::string>& namespacePath) const;
        size_t getVariableCount() const;
        
        void removeVariable(const std::string& varName);
        void removeVariableFromNamespace(const std::vector<std::string>& namespacePath, const std::string& varName);
    };
}
