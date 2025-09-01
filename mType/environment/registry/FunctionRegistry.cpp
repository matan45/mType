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
    

    bool FunctionRegistry::hasItem(const std::string& name) const
    {
        return hasFunction(name);
    }

    void FunctionRegistry::removeItem(const std::string& name)
    {
        globalFunctions.erase(name);
    }

    std::vector<std::string> FunctionRegistry::getAllItemNames() const
    {
        std::vector<std::string> names;
        
        for (const auto& [name, _] : globalFunctions)
        {
            names.push_back(name);
        }
        
        std::sort(names.begin(), names.end());
        return names;
    }

    size_t FunctionRegistry::getItemCount() const
    {
        return globalFunctions.size();
    }

    std::string FunctionRegistry::getComponentName() const
    {
        return "FunctionRegistry";
    }

    void FunctionRegistry::registerFunction(const std::string& name, std::shared_ptr<FunctionDefinition> functionDefinition)
    {
        globalFunctions[name] = functionDefinition;
    }
    

    std::shared_ptr<FunctionDefinition> FunctionRegistry::findFunction(const std::string& name) const
    {
        auto it = globalFunctions.find(name);
        return (it != globalFunctions.end()) ? it->second : nullptr;
    }

    bool FunctionRegistry::hasFunction(const std::string& name) const
    {
        return globalFunctions.find(name) != globalFunctions.end();
    }
    
}
