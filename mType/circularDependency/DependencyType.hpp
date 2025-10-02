#pragma once

namespace circularDependency
{
    enum class DependencyType
    {
        GENERIC_SUBSTITUTION,
        IMPORT_CHAIN,
        INTERFACE_INHERITANCE,
        CLASS_INHERITANCE,
        METHOD_OVERLOAD
    };
}