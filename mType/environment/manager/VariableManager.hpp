#pragma once
#include <unordered_map>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include "../../runtimeTypes/global/VariableDefinition.hpp"

namespace environment::manager
{
    using namespace runtimeTypes::global;

    /**
     * @brief Manages global variables only
     *
     * This class is responsible for managing global scope variables.
     * Local scope variables are handled by ScopeManager.
     *
     * Note: This class was previously responsible for scope management,
     * but that functionality has been moved to ScopeManager for better
     * separation of concerns.
     */
    class VariableManager
    {
    private:
        std::unordered_map<std::string, std::shared_ptr<VariableDefinition>> globalVariables;

    public:
        explicit VariableManager() = default;
        ~VariableManager() = default;

        std::string getComponentName() const;
        void cleanup();

        // Global variable management
        void declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable);
        std::shared_ptr<VariableDefinition> findVariable(const std::string& varName) const;
        bool hasVariable(const std::string& varName) const;

        std::vector<std::string> getAllVariableNames() const;
        size_t getVariableCount() const;

        void removeVariable(const std::string& varName);
    };
}
