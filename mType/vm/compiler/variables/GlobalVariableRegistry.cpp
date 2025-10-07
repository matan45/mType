#include "GlobalVariableRegistry.hpp"

namespace vm::compiler::variables
{
    void GlobalVariableRegistry::registerGlobal(const std::string& name, value::ValueType type,
                                               const std::string& className, int scopeDepth)
    {
        globalVariables.insert(name);
        globalVariableTypes[name] = type;
        globalVariableClassNames[name] = className;
        globalVariableScopes[name] = scopeDepth;
    }

    bool GlobalVariableRegistry::exists(const std::string& name) const
    {
        return globalVariables.find(name) != globalVariables.end();
    }

    value::ValueType GlobalVariableRegistry::getType(const std::string& name) const
    {
        auto it = globalVariableTypes.find(name);
        if (it != globalVariableTypes.end()) {
            return it->second;
        }
        return value::ValueType::VOID;
    }

    std::string GlobalVariableRegistry::getClassName(const std::string& name) const
    {
        auto it = globalVariableClassNames.find(name);
        if (it != globalVariableClassNames.end()) {
            return it->second;
        }
        return "";
    }

    int GlobalVariableRegistry::getScopeDepth(const std::string& name) const
    {
        auto it = globalVariableScopes.find(name);
        if (it != globalVariableScopes.end()) {
            return it->second;
        }
        return -1;
    }

    bool GlobalVariableRegistry::isInScope(const std::string& name, int currentScopeDepth) const
    {
        auto scopeIt = globalVariableScopes.find(name);
        if (scopeIt != globalVariableScopes.end()) {
            return scopeIt->second <= currentScopeDepth;
        }
        return false;
    }

    void GlobalVariableRegistry::removeVariablesOutOfScope(int currentScopeDepth)
    {
        std::vector<std::string> toRemove;
        for (const auto& pair : globalVariableScopes) {
            if (pair.second > currentScopeDepth) {
                toRemove.push_back(pair.first);
            }
        }
        for (const auto& name : toRemove) {
            globalVariables.erase(name);
            globalVariableTypes.erase(name);
            globalVariableClassNames.erase(name);
            globalVariableScopes.erase(name);
        }
    }

    void GlobalVariableRegistry::clear()
    {
        globalVariables.clear();
        globalVariableTypes.clear();
        globalVariableClassNames.clear();
        globalVariableScopes.clear();
    }
}
