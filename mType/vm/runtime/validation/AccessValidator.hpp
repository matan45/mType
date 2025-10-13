#pragma once
#include "../../../ast/AccessModifier.hpp"
#include "../../../errors/AccessViolationException.hpp"
#include "../../../runtimeTypes/klass/FieldDefinition.hpp"
#include "../../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include <string>
#include <memory>

namespace vm::runtime::validation
{
    /**
     * Context information for access validation
     * Contains information about the calling context and target
     */
    struct AccessContext
    {
        std::string callingClassName;    // Class making the access (empty for global scope)
        std::string targetClassName;     // Class being accessed
        bool isSameClass;                // Is calling class same as target class?
        bool isSubclass;                 // Is calling class a subclass of target class?
        bool isSetterAccess;             // Is this a setter access (for final field validation)?
        ast::SourceLocation location;    // Source location for error reporting

        AccessContext(
            const std::string& caller,
            const std::string& target,
            bool sameClass,
            bool subclass,
            bool setter = false,
            const ast::SourceLocation& loc = ast::SourceLocation()
        )
            : callingClassName(caller)
            , targetClassName(target)
            , isSameClass(sameClass)
            , isSubclass(subclass)
            , isSetterAccess(setter)
            , location(loc)
        {
        }
    };

    /**
     * Validates access control (public/private/protected) for class members
     * Used by VM runtime to enforce access modifiers at execution time
     */
    class AccessValidator
    {
    public:
        /**
         * Validates field access based on access modifier and context
         * @throws AccessViolationException if access is not allowed
         */
        static void validateFieldAccess(
            const std::string& fieldName,
            ast::AccessModifier accessModifier,
            const AccessContext& context
        );

        /**
         * Validates method access based on access modifier and context
         * @throws AccessViolationException if access is not allowed
         */
        static void validateMethodAccess(
            const std::string& methodName,
            ast::AccessModifier accessModifier,
            const AccessContext& context
        );

        /**
         * Validates constructor access based on access modifier and context
         * @throws AccessViolationException if access is not allowed
         */
        static void validateConstructorAccess(
            const std::string& className,
            ast::AccessModifier accessModifier,
            const AccessContext& context
        );

        /**
         * Checks if access is allowed based on modifier and context
         * @return true if access is allowed, false otherwise
         */
        static bool canAccess(
            ast::AccessModifier accessModifier,
            const AccessContext& context
        );

    private:
        /**
         * Internal method to determine if access is allowed
         * Implements the access control logic (PUBLIC/PRIVATE/PROTECTED)
         */
        static bool isAccessAllowed(
            ast::AccessModifier modifier,
            const AccessContext& context
        );
    };
}
