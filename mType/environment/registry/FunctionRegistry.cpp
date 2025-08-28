#include "FunctionRegistry.hpp"
#include <algorithm>
#include <sstream>

namespace environment::registry
{
    void FunctionRegistry::registerItem(const std::string& name, std::shared_ptr<FunctionDefinition> item)
    {
        registerFunction(name, item);
    }

    std::shared_ptr<FunctionDefinition> FunctionRegistry::findItem(const std::string& name) const
    {
        return findFunction(name);
    }

    std::shared_ptr<FunctionDefinition> FunctionRegistry::findQualifiedItem(const std::vector<std::string>& qualifiedName) const
    {
        if (qualifiedName.empty()) return nullptr;
        
        if (qualifiedName.size() == 1)
        {
            return findFunction(qualifiedName[0]);
        }
        
        std::vector<std::string> namespacePath(qualifiedName.begin(), qualifiedName.end() - 1);
        return findFunctionInNamespace(namespacePath, qualifiedName.back());
    }

    bool FunctionRegistry::hasItem(const std::string& name) const
    {
        return hasFunction(name);
    }

    void FunctionRegistry::removeItem(const std::string& name)
    {
        globalFunctions.erase(name);
        overloadedFunctions.erase(name);
        
        for (auto& [ns, nsFunctions] : namespacedFunctions)
        {
            nsFunctions.erase(name);
        }
    }

    std::vector<std::string> FunctionRegistry::getAllItemNames() const
    {
        std::vector<std::string> names;
        
        for (const auto& [name, _] : globalFunctions)
        {
            names.push_back(name);
        }
        
        for (const auto& [ns, nsFunctions] : namespacedFunctions)
        {
            for (const auto& [functionName, _] : nsFunctions)
            {
                names.push_back(ns + "::" + functionName);
            }
        }
        
        std::sort(names.begin(), names.end());
        return names;
    }

    size_t FunctionRegistry::getItemCount() const
    {
        size_t count = globalFunctions.size();
        for (const auto& [ns, nsFunctions] : namespacedFunctions)
        {
            count += nsFunctions.size();
        }
        return count;
    }

    std::string FunctionRegistry::getComponentName() const
    {
        return "FunctionRegistry";
    }

    void FunctionRegistry::registerFunction(const std::string& name, std::shared_ptr<FunctionDefinition> functionDefinition)
    {
        globalFunctions[name] = functionDefinition;
        
        overloadedFunctions[name].push_back(functionDefinition);
    }

    void FunctionRegistry::registerFunctionInNamespace(const std::vector<std::string>& namespacePath, const std::string& functionName, std::shared_ptr<FunctionDefinition> functionDefinition)
    {
        std::string nsPath;
        if (!namespacePath.empty())
        {
            std::ostringstream oss;
            for (size_t i = 0; i < namespacePath.size(); ++i)
            {
                if (i > 0) oss << "::";
                oss << namespacePath[i];
            }
            nsPath = oss.str();
        }
        
        namespacedFunctions[nsPath][functionName] = functionDefinition;
        functionDefinition->setNamespaceContext(namespacePath);
        
        std::string fullName = nsPath.empty() ? functionName : nsPath + "::" + functionName;
        overloadedFunctions[fullName].push_back(functionDefinition);
    }

    std::shared_ptr<FunctionDefinition> FunctionRegistry::findFunction(const std::string& name) const
    {
        auto it = globalFunctions.find(name);
        return (it != globalFunctions.end()) ? it->second : nullptr;
    }

    std::shared_ptr<FunctionDefinition> FunctionRegistry::findFunctionInNamespace(const std::vector<std::string>& namespacePath, const std::string& functionName) const
    {
        std::string nsPath;
        if (!namespacePath.empty())
        {
            std::ostringstream oss;
            for (size_t i = 0; i < namespacePath.size(); ++i)
            {
                if (i > 0) oss << "::";
                oss << namespacePath[i];
            }
            nsPath = oss.str();
        }
        
        auto nsIt = namespacedFunctions.find(nsPath);
        if (nsIt != namespacedFunctions.end())
        {
            auto funcIt = nsIt->second.find(functionName);
            if (funcIt != nsIt->second.end())
            {
                return funcIt->second;
            }
        }
        
        return findFunction(functionName);
    }

    std::shared_ptr<FunctionDefinition> FunctionRegistry::findFunctionWithSignature(const std::string& name, size_t argCount) const
    {
        auto it = overloadedFunctions.find(name);
        if (it != overloadedFunctions.end())
        {
            for (const auto& func : it->second)
            {
                if (func && func->getParameterCount() == argCount)
                {
                    return func;
                }
            }
        }
        
        return nullptr;
    }

    bool FunctionRegistry::hasFunction(const std::string& name) const
    {
        return globalFunctions.find(name) != globalFunctions.end();
    }

    std::vector<std::shared_ptr<FunctionDefinition>> FunctionRegistry::getFunctionOverloads(const std::string& name) const
    {
        auto it = overloadedFunctions.find(name);
        return (it != overloadedFunctions.end()) ? it->second : std::vector<std::shared_ptr<FunctionDefinition>>{};
    }

    std::vector<std::string> FunctionRegistry::getFunctionsInNamespace(const std::vector<std::string>& namespacePath) const
    {
        std::vector<std::string> functionNames;
        
        std::string nsPath;
        if (!namespacePath.empty())
        {
            std::ostringstream oss;
            for (size_t i = 0; i < namespacePath.size(); ++i)
            {
                if (i > 0) oss << "::";
                oss << namespacePath[i];
            }
            nsPath = oss.str();
        }
        
        auto nsIt = namespacedFunctions.find(nsPath);
        if (nsIt != namespacedFunctions.end())
        {
            for (const auto& [functionName, _] : nsIt->second)
            {
                functionNames.push_back(functionName);
            }
        }
        
        std::sort(functionNames.begin(), functionNames.end());
        return functionNames;
    }
}
