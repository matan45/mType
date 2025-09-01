#pragma once
#include "IRegistry.hpp"
#include "../../runtimeTypes/global/FunctionDefinition.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

namespace environment::registry
{
    using namespace runtimeTypes::global;

    class FunctionRegistry : public IRegistry<FunctionDefinition>
    {
    private:
        std::unordered_map<std::string, std::shared_ptr<FunctionDefinition>> globalFunctions;

    public:
        explicit FunctionRegistry() = default;
        ~FunctionRegistry() override = default;

        void registerItem(const std::string& name, std::shared_ptr<FunctionDefinition> item) override;
        std::shared_ptr<FunctionDefinition> findItem(const std::string& name) const override;
        bool hasItem(const std::string& name) const override;
        void removeItem(const std::string& name) override;
        std::vector<std::string> getAllItemNames() const override;
        size_t getItemCount() const override;
        
        std::string getComponentName() const override;
        
        void registerFunction(const std::string& name, std::shared_ptr<FunctionDefinition> functionDefinition);
        std::shared_ptr<FunctionDefinition> findFunction(const std::string& name) const;
        bool hasFunction(const std::string& name) const;
    };
}
