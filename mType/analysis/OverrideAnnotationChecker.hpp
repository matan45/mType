#pragma once

#include <vector>
#include "../diagnostics/Diagnostic.hpp"

namespace runtimeTypes::klass
{
    class ClassDefinition;
}

namespace analysis
{
    /**
     * Checks every instance method declared on a class for the
     * "missing @Override" warning condition (MYT-50, MT-W2002).
     *
     * A warning is emitted when:
     *   - The method does NOT carry the `@Override` annotation, AND
     *   - An ancestor class declares an instance method with the same
     *     name and signature, AND
     *   - That ancestor method is not `private` (private members aren't
     *     visible to subclasses, so the child isn't actually overriding).
     *
     * The function is **pure read-only** against the `ClassDefinition` —
     * it does not mutate the registry, the class, or any annotation. Both
     * the bytecode-compile path (`ClassRegistrar::validateMethodOverrides`)
     * and the LSP path (`DocumentManager::parseDocument`) call into it.
     *
     * Threading: safe to call from any thread as long as the
     * `ClassDefinition` is not being mutated concurrently. Both call
     * sites in the codebase satisfy that invariant.
     */
    class OverrideAnnotationChecker
    {
    public:
        static std::vector<diagnostics::Diagnostic> check(
            const runtimeTypes::klass::ClassDefinition& cls);
    };
}
