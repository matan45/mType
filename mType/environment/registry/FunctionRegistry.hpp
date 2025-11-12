#pragma once
#include "Registry.hpp"
#include "../../runtimeTypes/global/FunctionDefinition.hpp"
#include <memory>
#include <string>

namespace environment::registry
{
    using namespace runtimeTypes::global;

    class FunctionRegistry : public Registry<FunctionDefinition>
    {
    private:
        // NEW: Support overloading - store multiple functions per name
        std::unordered_map<std::string, std::vector<std::shared_ptr<FunctionDefinition>>> functionOverloads;

    public:
        explicit FunctionRegistry() = default;
        ~FunctionRegistry() = default;

        std::string getComponentName() const override;

        // Existing methods (backward compatibility)
        void registerFunction(const std::string& name, std::shared_ptr<FunctionDefinition> functionDefinition);
        std::shared_ptr<FunctionDefinition> findFunction(const std::string& name) const;
        bool hasFunction(const std::string& name) const;

        // NEW: Overload-specific methods
        // Get all overloads for a function name
        std::vector<std::shared_ptr<FunctionDefinition>> getAllFunctionOverloads(const std::string& name) const;

        // Find function by signature (with type parameters)
        std::shared_ptr<FunctionDefinition> findFunctionBySignature(
            const std::string& name,
            const std::vector<std::pair<std::string, value::ParameterType>>& parameters) const;

        // Find function by type signature string
        std::shared_ptr<FunctionDefinition> findFunctionByTypeSignature(
            const std::string& name,
            const std::string& typeSignature) const;

        // Find function by argument count
        std::shared_ptr<FunctionDefinition> findFunctionByArgCount(
            const std::string& name,
            size_t argCount) const;
    };
}
