#pragma once

#include "../../environment/registry/AnnotationRegistry.hpp"
#include <memory>

namespace validation::builtins
{
    /// Pre-registers Override/Script/EntryPoint/Throw as built-in
    /// AnnotationDefinitions. Their parameter SCHEMA lives here; their
    /// semantic enforcement (lifecycle methods, exception class verification)
    /// stays in the existing AnnotationValidator pipeline (per user
    /// requirement — @Script enforcement must remain in C++).
    class BuiltInAnnotations
    {
    public:
        static void registerAll(std::shared_ptr<environment::registry::AnnotationRegistry> registry);
    };
}
