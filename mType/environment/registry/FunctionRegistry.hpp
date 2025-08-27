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
        std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<FunctionDefinition>>> namespacedFunctions;
        std::unordered_map<std::string, std::vector<std::shared_ptr<FunctionDefinition>>> overloadedFunctions;

    public:
        explicit FunctionRegistry() = default;
        ~FunctionRegistry() override = default;

        void registerItem(const std::string& name, std::shared_ptr<FunctionDefinition> item) override;
        std::shared_ptr<FunctionDefinition> findItem(const std::string& name) const override;
        std::shared_ptr<FunctionDefinition> findQualifiedItem(const std::vector<std::string>& qualifiedName) const override;
        bool hasItem(const std::string& name) const override;
        void removeItem(const std::string& name) override;
        std::vector<std::string> getAllItemNames() const override;
        size_t getItemCount() const override;
        
        std::string getComponentName() const override;
        
        void registerFunction(const std::string& name, std::shared_ptr<FunctionDefinition> functionDefinition);
        void registerFunctionInNamespace(const std::vector<std::string>& namespacePath, const std::string& functionName, std::shared_ptr<FunctionDefinition> functionDefinition);
        std::shared_ptr<FunctionDefinition> findFunction(const std::string& name) const;
        std::shared_ptr<FunctionDefinition> findFunctionInNamespace(const std::vector<std::string>& namespacePath, const std::string& functionName) const;
        std::shared_ptr<FunctionDefinition> findFunctionWithSignature(const std::string& name, size_t argCount) const;
        bool hasFunction(const std::string& name) const;
        std::vector<std::shared_ptr<FunctionDefinition>> getFunctionOverloads(const std::string& name) const;
        std::vector<std::string> getFunctionsInNamespace(const std::vector<std::string>& namespacePath) const;
    };
}
