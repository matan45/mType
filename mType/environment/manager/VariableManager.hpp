#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include "IManager.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"


namespace environment::manager
{
    using namespace runtimeTypes::global;

    class VariableManager : public IManager
    {
    private:
        std::unordered_map<std::string, std::shared_ptr<VariableDefinition>> globalVariables;

    public:
        VariableManager() = default;
        ~VariableManager() override = default;
        
        std::string getComponentName() const override;
        void cleanup() override;
        
        void declareVariable(const std::string& varName, std::shared_ptr<VariableDefinition> variable);
        std::shared_ptr<VariableDefinition> findVariable(const std::string& varName) const;
        bool hasVariable(const std::string& varName) const;
        
        std::vector<std::string> getAllVariableNames() const;
        size_t getVariableCount() const;
        
        void removeVariable(const std::string& varName);
    };
}
