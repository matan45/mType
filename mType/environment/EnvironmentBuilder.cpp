#include "EnvironmentBuilder.hpp"
#include "registry/TypeCatalog.hpp"
#include "../runtimeTypes/global/ArrayOperationsNative.hpp"
#include "../reflection/ReflectionNatives.hpp"
#include "../json/JsonNatives.hpp"
#include "../net/NetNatives.hpp"
#include "../project/mtclib/LibraryNatives.hpp"
#include "../plugin/PluginNatives.hpp"
#include "registry/builtin/ObjectClassBootstrap.hpp"

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
            classRegistry = std::make_shared<TypeCatalog>();
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

        // Register built-in Object base class (must be first — all classes implicitly inherit from Object)
        registry::builtin::registerObjectClass(environment);

        // Register SIMD-accelerated array operations
        runtimeTypes::global::ArrayOperationsNative::registerAll(environment);

        // Register reflection native functions (MYT-189: ported to ValueShim,
        // registered on both flag paths).
        reflection::ReflectionNatives::registerAll(environment);

        // Register JSON serialization native functions (MYT-189: ported to
        // ValueShim, registered on both flag paths).
        json::JsonNatives::registerAll(environment);

        // Register networking native functions (HTTP, TCP, DNS) (MYT-189:
        // ported to ValueShim, registered on both flag paths).
        net::NetNatives::registerAll(environment);

        // Register library loading native functions
        auto libraryLoader = std::make_shared<project::mtclib::TransitiveDependencyLoader>();
        project::mtclib::LibraryNatives::registerAll(environment, libraryLoader);

        // Register native plugin loader (MYT-289): __plugin_load / __plugin_unload
        plugin::PluginNatives::registerAll(environment);

        return environment;
    }

    std::shared_ptr<Environment> EnvironmentBuilder::createDefault()
    {
        return EnvironmentBuilder().withDefaults().build();
    }
}
