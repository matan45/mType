#include "FunctionRegistry.hpp"

namespace environment::registry
{
    std::string FunctionRegistry::getComponentName() const
    {
        return "FunctionRegistry";
    }

    void FunctionRegistry::registerFunction(const std::string& name, std::shared_ptr<FunctionDefinition> functionDefinition)
    {
        registerItem(name, functionDefinition);
    }

    std::shared_ptr<FunctionDefinition> FunctionRegistry::findFunction(const std::string& name) const
    {
        return findItem(name);
    }

    bool FunctionRegistry::hasFunction(const std::string& name) const
    {
        return hasItem(name);
    }
}
