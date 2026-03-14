#pragma once
#include <memory>

namespace environment
{
    class Environment;
}

namespace environment::registry::builtin
{
    /**
     * @brief Bootstraps the built-in Object class definition
     *
     * Registers the root Object ClassDefinition with default methods
     * (toString, equals, hashCode) into the ClassRegistry.
     * This must be called before any user classes are registered.
     */
    class ObjectClassBootstrap
    {
    public:
        static void registerObjectClass(std::shared_ptr<environment::Environment> environment);
    };
}
