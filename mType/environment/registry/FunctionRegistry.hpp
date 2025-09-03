#pragma once
#include "../../runtimeTypes/global/FunctionDefinition.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

namespace environment::registry
{
    using namespace runtimeTypes::global;

    class FunctionRegistry
    {
    private:
        std::unordered_map<std::string, std::shared_ptr<FunctionDefinition>> globalFunctions;

    public:
        explicit FunctionRegistry() = default;
        ~FunctionRegistry() = default;

        void registerItem(const std::string& name, std::shared_ptr<FunctionDefinition> item);
        std::shared_ptr<FunctionDefinition> findItem(const std::string& name) const;
        bool hasItem(const std::string& name) const;
        void removeItem(const std::string& name);
        std::vector<std::string> getAllItemNames() const;
        size_t getItemCount() const;
        
        std::string getComponentName() const;
        
        void registerFunction(const std::string& name, std::shared_ptr<FunctionDefinition> functionDefinition);
        std::shared_ptr<FunctionDefinition> findFunction(const std::string& name) const;
        bool hasFunction(const std::string& name) const;
    };
}
