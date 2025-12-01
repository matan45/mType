#include "EnvironmentBuilder.hpp"
#include "../runtimeTypes/global/ArrayOperationsNative.hpp"
#include "../reflection/ReflectionNatives.hpp"

namespace environment
{
    EnvironmentBuilder::EnvironmentBuilder()
    {
    }

    EnvironmentBuilder& EnvironmentBuilder::withClassRegistry(std::shared_ptr<ClassRegistry> registry)
    {
        classRegistry = registry;
        return *this;
    }

    EnvironmentBuilder& EnvironmentBuilder::withFunctionRegistry(std::shared_ptr<FunctionRegistry> registry)
    {
        functionRegistry = registry;
        return *this;
    }

    EnvironmentBuilder& EnvironmentBuilder::withVariableManager(std::shared_ptr<VariableManager> manager)
    {
        variableManager = manager;
        return *this;
    }

    EnvironmentBuilder& EnvironmentBuilder::withScopeManager(std::shared_ptr<ScopeManager> manager)
    {
        scopeManager = manager;
        return *this;
    }

    EnvironmentBuilder& EnvironmentBuilder::withNativeRegistry(std::shared_ptr<NativeRegistry> registry)
    {
        nativeRegistry = registry;
        return *this;
    }

    EnvironmentBuilder& EnvironmentBuilder::withDefaults()
    {
        if (!classRegistry)
            classRegistry = std::make_shared<ClassRegistry>();
        if (!functionRegistry)
            functionRegistry = std::make_shared<FunctionRegistry>();
        if (!variableManager)
            variableManager = std::make_shared<VariableManager>();
        if (!scopeManager)
            scopeManager = std::make_shared<ScopeManager>();
        if (!nativeRegistry)
            nativeRegistry = std::make_shared<NativeRegistry>();

        return *this;
    }

    std::shared_ptr<Environment> EnvironmentBuilder::build()
    {
        withDefaults();

        auto environment = std::make_shared<Environment>(
            classRegistry,
            functionRegistry,
            variableManager,
            scopeManager,
            nativeRegistry
        );

        environment->initialize();

        // Register SIMD-accelerated array operations
        runtimeTypes::global::ArrayOperationsNative::registerAll(environment);

        // Register reflection native functions
        reflection::ReflectionNatives::registerAll(environment);

        return environment;
    }

    std::shared_ptr<Environment> EnvironmentBuilder::createDefault()
    {
        return EnvironmentBuilder().withDefaults().build();
    }
}
