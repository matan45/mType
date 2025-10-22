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
        return globalVariables.find(varName) != globalVariables.end();
    }

    std::vector<std::string> VariableManager::getAllVariableNames() const
    {
        std::vector<std::string> names;
        names.reserve(globalVariables.size());

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
}
