#pragma once
#include "Environment.hpp"
#include "registry/ClassRegistry.hpp"
#include "registry/FunctionRegistry.hpp"
#include "registry/NativeRegistry.hpp"
#include "manager/VariableManager.hpp"
#include "manager/ScopeManager.hpp"
#include <memory>

namespace environment
{
    class EnvironmentBuilder
    {
    private:
        std::shared_ptr<ClassRegistry> classRegistry;
        std::shared_ptr<FunctionRegistry> functionRegistry;
        std::shared_ptr<VariableManager> variableManager;
        std::shared_ptr<ScopeManager> scopeManager;
        std::shared_ptr<NativeRegistry> nativeRegistry;

    public:
        explicit EnvironmentBuilder();
        
        EnvironmentBuilder& withClassRegistry(std::shared_ptr<ClassRegistry> registry);
        EnvironmentBuilder& withFunctionRegistry(std::shared_ptr<FunctionRegistry> registry);
        EnvironmentBuilder& withVariableManager(std::shared_ptr<VariableManager> manager);
        EnvironmentBuilder& withScopeManager(std::shared_ptr<ScopeManager> manager);
        EnvironmentBuilder& withNativeRegistry(std::shared_ptr<NativeRegistry> registry);
        
        EnvironmentBuilder& withDefaults();
        
        std::shared_ptr<Environment> build();
        
        static std::shared_ptr<Environment> createDefault();
    };
}