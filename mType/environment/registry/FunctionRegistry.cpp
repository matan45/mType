#include "FunctionRegistry.hpp"

namespace environment::registry
{
    std::string FunctionRegistry::getComponentName() const
    {
        return "FunctionRegistry";
    }

    void FunctionRegistry::registerFunction(const std::string& name, std::shared_ptr<FunctionDefinition> functionDefinition)
    {
        // NEW: Support overloading - append to vector of overloads
        functionOverloads[name].push_back(functionDefinition);
        // Also register in base class for backward compatibility
        registerItem(name, functionDefinition);
    }

    std::shared_ptr<FunctionDefinition> FunctionRegistry::findFunction(const std::string& name) const
    {
        // Return first overload for backward compatibility
        auto it = functionOverloads.find(name);
        if (it != functionOverloads.end() && !it->second.empty()) {
            return it->second[0];
        }
        return nullptr;
    }

    bool FunctionRegistry::hasFunction(const std::string& name) const
    {
        return functionOverloads.find(name) != functionOverloads.end();
    }

    // NEW: Overload-specific method implementations
    std::vector<std::shared_ptr<FunctionDefinition>> FunctionRegistry::getAllFunctionOverloads(const std::string& name) const
    {
        auto it = functionOverloads.find(name);
        if (it != functionOverloads.end()) {
            return it->second;
        }
        return {};
    }

    std::shared_ptr<FunctionDefinition> FunctionRegistry::findFunctionBySignature(
        const std::string& name,
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters) const
    {
        auto it = functionOverloads.find(name);
        if (it == functionOverloads.end()) {
            return nullptr;
        }

        // Search all overloads for exact signature match
        for (const auto& func : it->second) {
            if (!func) continue;

            const auto& funcParams = func->getParametersWithTypes();
            if (funcParams.size() != parameters.size()) {
                continue;
            }

            // Check if all parameter types match
            bool allMatch = true;
            for (size_t i = 0; i < parameters.size(); ++i) {
                if (!(funcParams[i].second == parameters[i].second)) {
                    allMatch = false;
                    break;
                }
            }

            if (allMatch) {
                return func;
            }
        }

        return nullptr;
    }

    std::shared_ptr<FunctionDefinition> FunctionRegistry::findFunctionByTypeSignature(
        const std::string& name,
        const std::string& typeSignature) const
    {
        auto it = functionOverloads.find(name);
        if (it == functionOverloads.end()) {
            return nullptr;
        }

        // Search all overloads for matching type signature
        for (const auto& func : it->second) {
            if (!func) continue;

            // Generate signature for this function and compare
            std::string funcSig;
            const auto& params = func->getParametersWithTypes();
            for (size_t i = 0; i < params.size(); ++i) {
                if (i > 0) funcSig += ",";
                funcSig += params[i].second.toString();
            }

            if (funcSig == typeSignature) {
                return func;
            }
        }

        return nullptr;
    }

    std::shared_ptr<FunctionDefinition> FunctionRegistry::findFunctionByArgCount(
        const std::string& name,
        size_t argCount) const
    {
        auto it = functionOverloads.find(name);
        if (it == functionOverloads.end()) {
            return nullptr;
        }

        // Search all overloads for matching parameter count
        for (const auto& func : it->second) {
            if (func && func->getParameters().size() == argCount) {
                return func;
            }
        }

        return nullptr;
    }
}
