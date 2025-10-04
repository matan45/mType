#pragma once

#include "../base/AccessContext.hpp"
#include "../../ast/AccessModifier.hpp"
#include "../../runtimeTypes/klass/FieldDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../runtimeTypes/klass/ConstructorDefinition.hpp"
#include <memory>

namespace evaluator::validation
{
    using namespace ast;
    using namespace base;

    /**
     * @brief Validates access control rules at runtime
     *
     * Single Responsibility: Only access control validation
     * Stateless: All context passed as parameters
     * Pure Logic: No side effects, only validation (throws on violation)
     */
    class AccessValidator
    {
    public:
        /**
         * @brief Validate field access
         * @param context The access context (who is accessing from where)
         * @param field The field being accessed
         * @throws AccessViolationException if access is denied
         */
        static void validateFieldAccess(
            const AccessContext& context,
            const runtimeTypes::klass::FieldDefinition& field);

        /**
         * @brief Validate method invocation
         * @param context The access context
         * @param method The method being invoked
         * @throws AccessViolationException if access is denied
         */
        static void validateMethodAccess(
            const AccessContext& context,
            const runtimeTypes::klass::MethodDefinition& method);

        /**
         * @brief Validate constructor invocation
         * @param context The access context
         * @param constructor The constructor being invoked
         * @throws AccessViolationException if access is denied
         */
        static void validateConstructorAccess(
            const AccessContext& context,
            const runtimeTypes::klass::ConstructorDefinition& constructor);

        /**
         * @brief Check if access is allowed (no exception version)
         * @param context The access context
         * @param memberModifier The access modifier of the member
         * @return true if access allowed, false otherwise
         */
        static bool canAccess(
            const AccessContext& context,
            AccessModifier memberModifier);

    private:
        /**
         * @brief Core access checking logic
         * @param modifier The access modifier to check
         * @param context The access context
         * @return true if access is allowed
         */
        static bool isAccessAllowed(
            AccessModifier modifier,
            const AccessContext& context);
    };
}
