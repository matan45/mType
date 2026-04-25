#pragma once
#include <memory>

namespace environment
{
    class Environment;
}

namespace environment::registry::builtin
{
    /**
     * @brief Registers the built-in Object ClassDefinition
     *
     * Creates the root Object class with default methods
     * (toString, equals, hashCode) in the ClassRegistry.
     * Must be called before any user classes are registered.
     *
     * @throws std::runtime_error if ClassRegistry is not available
     */
    void registerObjectClass(const std::shared_ptr<environment::Environment>& environment);
}

