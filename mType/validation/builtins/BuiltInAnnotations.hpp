#pragma once

#include "../../environment/registry/AnnotationRegistry.hpp"
#include <memory>
#include <string>
#include <vector>

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

    private:
        /// Registers a zero-parameter marker annotation with a @Target meta
        /// restricting it to the given host kinds (e.g. {"CLASS"}). No-op if a
        /// definition with this name is already registered.
        static void registerMarker(
            std::shared_ptr<environment::registry::AnnotationRegistry> registry,
            const std::string& name,
            const std::vector<std::string>& targets);
    };
}
