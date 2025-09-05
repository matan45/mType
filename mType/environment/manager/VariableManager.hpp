#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include "../../runtimeTypes/global/VariableDefinition.hpp"


namespace environment::manager
{
    using namespace runtimeTypes::global;

    class VariableManager
    {
    private:
        std::unordered_map<std::string, std::shared_ptr<VariableDefinition>> globalVariables;

    public:
        explicit VariableManager() = default;
        ~VariableManager() = default;
        
        std::string getComponentName() const;
        void cleanup();
        
        // Scope management methods (no-op since namespaces removed)
        void enterScope(const std::string& scopeName = "");
        void exitScope();
        std::string getCurrentScopeName() const;
        std::vector<std::string> getScopeHierarchy() const;
        size_t getScopeDepth() const;
        
        void declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable);
        std::shared_ptr<VariableDefinition> findVariable(const std::string& varName) const;
        bool hasVariable(const std::string& varName) const;
        
        std::vector<std::string> getAllVariableNames() const;
        size_t getVariableCount() const;
        
        void removeVariable(const std::string& varName);
    };
}
