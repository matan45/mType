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
    public:
        explicit FunctionRegistry() = default;
        ~FunctionRegistry() = default;

        std::string getComponentName() const override;

        void registerFunction(const std::string& name, std::shared_ptr<FunctionDefinition> functionDefinition);
        std::shared_ptr<FunctionDefinition> findFunction(const std::string& name) const;
        bool hasFunction(const std::string& name) const;
    };
}
