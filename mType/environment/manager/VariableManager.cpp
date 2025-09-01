#include "VariableManager.hpp"
#include <algorithm>

namespace environment::manager
{
    

    std::string VariableManager::getComponentName() const
    {
        return "VariableManager";
    }
    

    void VariableManager::cleanup()
    {
        globalVariables.clear();
    }

    void VariableManager::declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable)
    {
        globalVariables[varName] = variable;
    }

   

    std::shared_ptr<VariableDefinition> VariableManager::findVariable(const std::string& varName) const
    {
        auto it = globalVariables.find(varName);
        return (it != globalVariables.end()) ? it->second : nullptr;
    }

   
    bool VariableManager::hasVariable(const std::string& varName) const
    {
        return findVariable(varName) != nullptr;
    }
    

    std::vector<std::string> VariableManager::getAllVariableNames() const
    {
        std::vector<std::string> names;
        
        for (const auto& [name, _] : globalVariables)
        {
            names.push_back(name);
        }
        
        std::sort(names.begin(), names.end());
        return names;
    }

   

    size_t VariableManager::getVariableCount() const
    {
       
        return globalVariables.size();
    }

    void VariableManager::removeVariable(const std::string& varName)
    {
        globalVariables.erase(varName);
    }

    void VariableManager::enterScope(const std::string& scopeName)
    {
        // No-op since we don't support namespaces anymore
    }

    void VariableManager::exitScope()
    {
        // No-op since we don't support namespaces anymore
    }

    std::string VariableManager::getCurrentScopeName() const
    {
        // Return empty string since we don't support namespaces anymore
        return "";
    }

    std::vector<std::string> VariableManager::getScopeHierarchy() const
    {
        // Return empty vector since we don't support namespaces anymore
        return {};
    }

    size_t VariableManager::getScopeDepth() const
    {
        // Return 0 since we don't support namespaces anymore
        return 0;
    }
    
}
