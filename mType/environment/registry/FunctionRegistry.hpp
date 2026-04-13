#pragma once
#include "Registry.hpp"
#include "../../runtimeTypes/global/FunctionDefinition.hpp"
#include <memory>
#include <string>

namespace environment::registry
{
    using namespace runtimeTypes::global;

    /**
     * Registry for global functions with support for function overloading.
     *
     * DESIGN NOTE: This class inherits from Registry<FunctionDefinition> for structural
     * consistency with other registries, but uses its own storage (functionOverloads)
     * to support multiple functions with the same name but different signatures.
     * The base class storage (items) is NOT used to avoid data duplication.
     */
    class FunctionRegistry : public Registry<FunctionDefinition>
    {
    private:
        // Support overloading - store multiple functions per name
        // This is the ONLY storage; base class Registry::items is NOT used
        std::unordered_map<std::string, std::vector<std::shared_ptr<FunctionDefinition>>> functionOverloads;

    public:
        explicit FunctionRegistry() = default;
        ~FunctionRegistry() = default;

        std::string getComponentName() const override;

        // Core registration and lookup methods
        void registerFunction(const std::string& name, std::shared_ptr<FunctionDefinition> functionDefinition);
        std::shared_ptr<FunctionDefinition> findFunction(const std::string& name) const;
        bool hasFunction(const std::string& name) const;

        // NEW: Overload-specific methods
        // Get all overloads for a function name
        std::vector<std::shared_ptr<FunctionDefinition>> getAllFunctionOverloads(const std::string& name) const;

        // Enumerate every registered function name (one entry per name,
        // not per overload). Sorted alphabetically for stable diagnostics.
        // Used by the diagnostic IdentifierEnumerator to build "did you
        // mean" pools.
        std::vector<std::string> getAllFunctionNames() const;

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

        // Remove all overloads for a function name
        bool removeFunction(const std::string& name);

        // Clear all script-defined functions
        void clearAllFunctions();

    private:
        // Hide base class methods to prevent accidental misuse of base storage
        // (base class storage is not used; all data is in functionOverloads)
        using Registry<FunctionDefinition>::registerItem;
        using Registry<FunctionDefinition>::findItem;
        using Registry<FunctionDefinition>::hasItem;
    };
}
