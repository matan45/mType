#include "VariableManager.hpp"
#include <algorithm>

namespace environment::manager
{
    void VariableManager::enterScope(const std::string& scopeName)
    {
        if (!scopeName.empty())
        {
            currentNamespacePath.push_back(scopeName);
        }
    }

    void VariableManager::exitScope()
    {
        if (!currentNamespacePath.empty())
        {
            currentNamespacePath.pop_back();
        }
    }

    std::string VariableManager::getCurrentScopeName() const
    {
        return currentNamespacePath.empty() ? "global" : currentNamespacePath.back();
    }

    std::vector<std::string> VariableManager::getScopeHierarchy() const
    {
        return currentNamespacePath;
    }

    size_t VariableManager::getScopeDepth() const
    {
        return currentNamespacePath.size();
    }

    std::string VariableManager::getComponentName() const
    {
        return "VariableManager";
    }

    void VariableManager::initialize()
    {
        globalVariables.clear();
        namespacedVariables.clear();
        currentNamespacePath.clear();
    }

    void VariableManager::cleanup()
    {
        globalVariables.clear();
        namespacedVariables.clear();
        currentNamespacePath.clear();
    }

    void VariableManager::declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable)
    {
        if (currentNamespacePath.empty())
        {
            globalVariables[varName] = variable;
        }
        else
        {
            declareVariableInNamespace(currentNamespacePath, varName, variable);
        }
    }

    void VariableManager::declareVariableInNamespace(const std::vector<std::string>& namespacePath, const std::string& varName, std::shared_ptr<VariableDefinition> variable)
    {
        std::string nsPath = utils::NamespaceUtils::vectorToPath(namespacePath);
        namespacedVariables[nsPath][varName] = variable;
        
        if (variable)
        {
            variable->setNamespaceContext(namespacePath);
        }
    }

    std::shared_ptr<VariableDefinition> VariableManager::findVariable(const std::string& varName) const
    {
        if (!currentNamespacePath.empty())
        {
            if (auto var = findVariableInNamespace(currentNamespacePath, varName))
            {
                return var;
            }
        }
        
        auto it = globalVariables.find(varName);
        return (it != globalVariables.end()) ? it->second : nullptr;
    }

    std::shared_ptr<VariableDefinition> VariableManager::findVariableInNamespace(const std::vector<std::string>& namespacePath, const std::string& varName) const
    {
        std::string nsPath = utils::NamespaceUtils::vectorToPath(namespacePath);
        
        auto nsIt = namespacedVariables.find(nsPath);
        if (nsIt != namespacedVariables.end())
        {
            auto varIt = nsIt->second.find(varName);
            if (varIt != nsIt->second.end())
            {
                return varIt->second;
            }
        }
        
        if (!namespacePath.empty())
        {
            std::vector<std::string> parentPath(namespacePath.begin(), namespacePath.end() - 1);
            return findVariableInNamespace(parentPath, varName);
        }
        
        auto it = globalVariables.find(varName);
        return (it != globalVariables.end()) ? it->second : nullptr;
    }

    bool VariableManager::hasVariable(const std::string& varName) const
    {
        return findVariable(varName) != nullptr;
    }

    bool VariableManager::hasVariableInNamespace(const std::vector<std::string>& namespacePath, const std::string& varName) const
    {
        return findVariableInNamespace(namespacePath, varName) != nullptr;
    }

    void VariableManager::setCurrentNamespace(const std::vector<std::string>& namespacePath)
    {
        currentNamespacePath = namespacePath;
    }

    const std::vector<std::string>& VariableManager::getCurrentNamespace() const
    {
        return currentNamespacePath;
    }

    std::vector<std::string> VariableManager::getAllVariableNames() const
    {
        std::vector<std::string> names;
        
        for (const auto& [name, _] : globalVariables)
        {
            names.push_back(name);
        }
        
        for (const auto& [ns, nsVars] : namespacedVariables)
        {
            for (const auto& [varName, _] : nsVars)
            {
                names.push_back(ns.empty() ? varName : ns + "::" + varName);
            }
        }
        
        std::sort(names.begin(), names.end());
        return names;
    }

    std::vector<std::string> VariableManager::getVariablesInNamespace(const std::vector<std::string>& namespacePath) const
    {
        std::vector<std::string> varNames;
        std::string nsPath = utils::NamespaceUtils::vectorToPath(namespacePath);
        
        auto nsIt = namespacedVariables.find(nsPath);
        if (nsIt != namespacedVariables.end())
        {
            for (const auto& [varName, _] : nsIt->second)
            {
                varNames.push_back(varName);
            }
        }
        
        std::sort(varNames.begin(), varNames.end());
        return varNames;
    }

    size_t VariableManager::getVariableCount() const
    {
        size_t count = globalVariables.size();
        for (const auto& [ns, nsVars] : namespacedVariables)
        {
            count += nsVars.size();
        }
        return count;
    }

    void VariableManager::removeVariable(const std::string& varName)
    {
        globalVariables.erase(varName);
        
        for (auto& [ns, nsVars] : namespacedVariables)
        {
            nsVars.erase(varName);
        }
    }

    void VariableManager::removeVariableFromNamespace(const std::vector<std::string>& namespacePath, const std::string& varName)
    {
        std::string nsPath = utils::NamespaceUtils::vectorToPath(namespacePath);
        
        auto nsIt = namespacedVariables.find(nsPath);
        if (nsIt != namespacedVariables.end())
        {
            nsIt->second.erase(varName);
        }
    }
}
