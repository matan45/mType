#pragma once

#include "../../ast/AccessModifier.hpp"
#include "../../errors/SourceLocation.hpp"
#include <string>
#include <memory>

// Forward declarations
namespace runtimeTypes::klass { class ClassDefinition; }
namespace runtimeTypes::klass { class ObjectInstance; }

namespace evaluator::base
{
    using namespace ast;
    using namespace errors;

    /**
     * @brief Tracks the calling context for access control validation
     *
     * This structure captures:
     * - The class where access is being attempted FROM (calling class)
     * - The class being accessed (target class)
     * - Whether the access is through inheritance
     * - Current execution scope (instance vs static)
     */
    struct AccessContext
    {
        // Calling context
        std::string callingClassName;           // Class making the access
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> callingClass;

        // Target context
        std::string targetClassName;            // Class being accessed
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> targetClass;

        // Access characteristics
        bool isStaticAccess = false;            // Static vs instance access
        bool isSameClass = false;               // Computed: calling == target
        bool isSubclass = false;                // Computed: calling extends target

        // Location for error reporting
        SourceLocation location;

        /**
         * @brief Create context for instance member access
         * @param callingInstance The instance from which access is attempted (can be null for global)
         * @param targetClassDef The class definition being accessed
         * @param loc Source location for error reporting
         * @return Initialized AccessContext
         */
        static AccessContext forInstanceAccess(
            std::shared_ptr<runtimeTypes::klass::ObjectInstance> callingInstance,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> targetClassDef,
            const SourceLocation& loc);

        /**
         * @brief Create context for static member access
         * @param callingClass The class name from which access is attempted
         * @param targetClassDef The class definition being accessed
         * @param loc Source location for error reporting
         * @param callingClassDef Optional calling class definition for inheritance checking
         * @return Initialized AccessContext
         */
        static AccessContext forStaticAccess(
            const std::string& callingClass,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> targetClassDef,
            const SourceLocation& loc,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> callingClassDef = nullptr);

        /**
         * @brief Create context for global scope access (no calling class)
         * @param targetClassDef The class definition being accessed
         * @param loc Source location for error reporting
         * @return Initialized AccessContext
         */
        static AccessContext forGlobalAccess(
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> targetClassDef,
            const SourceLocation& loc);
    };
}
